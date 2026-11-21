/**************************************************************************/
/*  font.cpp                                                              */
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

#include "core/config/engine.h"
#include "core/io/image_loader.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/types.h"
#include "font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/renderer.h"

/*************************************************************************/
/*  Font                                                                 */
/*************************************************************************/

bool Font::_is_base_cyclic(const Ref<Font>& p_f, int p_depth) const
{
	ERR_FAIL_COND_V(p_depth > MAX_FALLBACK_DEPTH, true);
	if (p_f.is_null()) {
		return false;
	}
	if (p_f == this) {
		return true;
	}
	Ref<FontVariation> fv = p_f;
	if (fv.is_valid()) {
		return _is_base_cyclic(fv->get_base_font(), p_depth + 1);
	}
	Ref<SystemFont> fs = p_f;
	if (fs.is_valid()) {
		return _is_base_cyclic(fs->get_base_font(), p_depth + 1);
	}
	return false;
}

static float _measure_glyph_run(
	const RID& p_font_rid, const String& p_text, int p_font_size, int p_len)
{
	float total_w = 0.0f;
	int64_t prev_glyph = 0;
	for (int i = 0; i < p_len; i++) {
		char32_t c = p_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(p_font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			total_w += TS->font_get_kerning(
							 p_font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						   .x;
		}
		total_w += TS->font_get_glyph_advance(p_font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
	return total_w;
}

real_t Font::draw_char_outline(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
	int p_font_size, int p_size, const Color& p_modulate, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	TS->font_draw_glyph_outline(
		font_rid, p_canvas_item, p_font_size, p_size, p_pos, glyph, p_modulate, p_oversampling);
	return TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
}

real_t Font::draw_char(RID p_canvas_item, const Point2& p_pos, char32_t p_char, int p_font_size,
	const Color& p_modulate, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	TS->font_draw_glyph(
		font_rid, p_canvas_item, p_font_size, p_pos, glyph, p_modulate, p_oversampling);
	return TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
}

void Font::draw_multiline_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_max_lines,
	const Color& p_modulate, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	Vector<String> lines = p_text.split("\n");
	float line_h = MAX(1.0f, get_height(p_font_size));
	Point2 cur_pos = p_pos;
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	for (int i = 0; i < count; i++) {
		draw_string(p_canvas_item, cur_pos, lines[i], p_alignment, p_width, p_font_size, p_modulate,
			p_jst_flags, p_direction, p_orientation, p_oversampling);
		cur_pos.y += line_h;
	}
}

void Font::draw_multiline_string_outline(RID p_canvas_item, const Point2& p_pos,
	const String& p_text, HorizontalAlignment p_alignment, float p_width, int p_font_size,
	int p_max_lines, int p_size, const Color& p_modulate, uint32_t p_brk_flags,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	Vector<String> lines = p_text.split("\n");
	float line_h = MAX(1.0f, get_height(p_font_size));
	Point2 cur_pos = p_pos;
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	for (int i = 0; i < count; i++) {
		draw_string_outline(p_canvas_item, cur_pos, lines[i], p_alignment, p_width, p_font_size,
			p_size, p_modulate, p_jst_flags, p_direction, p_orientation, p_oversampling);
		cur_pos.y += line_h;
	}
}

void Font::draw_string_outline(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_size,
	const Color& p_modulate, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty() || p_font_size <= 0) {
		return;
	}
	if (p_width == 0.0f) {
		return;
	}

	String draw_text = p_text;
	float text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
	if (p_width > 0.0f && text_w > p_width) {
		const String ellipsis = "...";
		const float ellipsis_w = _measure_glyph_run(font_rid, ellipsis, p_font_size, 3);
		if (ellipsis_w <= p_width) {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) + ellipsis_w >
					   p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep) + ellipsis;
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
		else {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) > p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep);
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
	}

	Point2 ofs = p_pos;
	if (p_width > 0.0f && text_w < p_width) {
		if (p_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			ofs.x += Math::floor((p_width - text_w) * 0.5f);
		}
		else if (p_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			ofs.x += Math::floor(p_width - text_w);
		}
	}

	int64_t prev_glyph = 0;
	for (int i = 0; i < draw_text.length(); i++) {
		char32_t c = draw_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			ofs.x += TS->font_get_kerning(
						   font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						 .x;
		}
		if (glyph != 0) {
			TS->font_draw_glyph_outline(font_rid, p_canvas_item, p_font_size, p_size, ofs, glyph,
				p_modulate, p_oversampling);
		}
		ofs.x += TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
}

void Font::draw_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, const Color& p_modulate,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty() || p_font_size <= 0) {
		return;
	}
	if (p_width == 0.0f) {
		return;
	}

	String draw_text = p_text;
	float text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
	if (p_width > 0.0f && text_w > p_width) {
		const String ellipsis = "...";
		const float ellipsis_w = _measure_glyph_run(font_rid, ellipsis, p_font_size, 3);
		if (ellipsis_w <= p_width) {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) + ellipsis_w >
					   p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep) + ellipsis;
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
		else {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) > p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep);
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
	}

	Point2 ofs = p_pos;
	if (p_width > 0.0f && text_w < p_width) {
		if (p_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			ofs.x += Math::floor((p_width - text_w) * 0.5f);
		}
		else if (p_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			ofs.x += Math::floor(p_width - text_w);
		}
	}

	int64_t prev_glyph = 0;
	for (int i = 0; i < draw_text.length(); i++) {
		char32_t c = draw_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			ofs.x += TS->font_get_kerning(
						   font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						 .x;
		}
		if (glyph != 0) {
			TS->font_draw_glyph(
				font_rid, p_canvas_item, p_font_size, ofs, glyph, p_modulate, p_oversampling);
		}
		ofs.x += TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
}

real_t Font::get_ascent(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_ascent(font_rid, p_font_size) + get_spacing(TextServer::SPACING_TOP);
}

Size2 Font::get_char_size(char32_t p_char, int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return Size2();
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	return Size2(
		TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x, get_height(p_font_size));
}

real_t Font::get_descent(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_descent(font_rid, p_font_size) + get_spacing(TextServer::SPACING_BOTTOM);
}

real_t Font::get_height(int p_font_size) const
{
	return get_ascent(p_font_size) + get_descent(p_font_size);
}

Size2 Font::get_multiline_string_size(const String& p_text, HorizontalAlignment p_alignment,
	float p_width, int p_font_size, int p_max_lines, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty()) {
		return Size2();
	}
	Vector<String> lines = p_text.split("\n");
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	float max_w = 0.0f;
	for (int i = 0; i < count; i++) {
		max_w = MAX(max_w, _measure_glyph_run(font_rid, lines[i], p_font_size, lines[i].length()));
	}
	if (p_width > 0.0f) {
		max_w = MIN(max_w, p_width);
	}
	return Size2(max_w, get_height(p_font_size) * count);
}

Size2 Font::get_string_size(const String& p_text, HorizontalAlignment p_alignment, float p_width,
	int p_font_size, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return Size2();
	}
	float w = _measure_glyph_run(font_rid, p_text, p_font_size, p_text.length());
	if (p_width > 0.0f) {
		w = MIN(w, p_width);
	}
	return Size2(w, get_height(p_font_size));
}

String Font::get_supported_chars() const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return String();
	}
	return TS->font_get_supported_chars(font_rid);
}

real_t Font::get_underline_position(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_underline_position(font_rid, p_font_size) +
		   get_spacing(TextServer::SPACING_TOP);
}

real_t Font::get_underline_thickness(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_underline_thickness(font_rid, p_font_size);
}

bool Font::has_char(char32_t p_char) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return false;
	}
	return TS->font_has_char(font_rid, p_char);
}

void Font::_invalidate_rids() {}

bool Font::_is_cyclic(const Ref<Font>& p_f, int p_depth) const { return true; }

void Font::_update_rids_fb(const Font* p_f, int p_depth) const {}

void Font::_update_rids() const {}

void Font::reset_state() { _invalidate_rids(); }

String Font::get_font_name() const { return TS->font_get_name(_get_rid()); }

int64_t Font::get_palette_count() const { return TS->font_get_palette_count(_get_rid()); }

String Font::get_palette_name(int64_t p_index) const
{
	return TS->font_get_palette_name(_get_rid(), p_index);
}

Vector<Color> Font::get_palette_colors(int64_t p_index) const
{
	return TS->font_get_palette_colors(_get_rid(), p_index);
}

String Font::get_font_style_name() const { return TS->font_get_style_name(_get_rid()); }

uint32_t Font::get_font_style() const { return TS->font_get_style(_get_rid()); }

int Font::get_font_weight() const { return TS->font_get_weight(_get_rid()); }

int Font::get_font_stretch() const { return TS->font_get_stretch(_get_rid()); }

// Drawing string.
void Font::set_cache_capacity(int p_single_line, int p_multi_line)
{
	cache.set_capacity(p_single_line);
	cache_wrap.set_capacity(p_multi_line);
}

bool Font::is_language_supported(const String& p_language) const
{
	return TS->font_is_language_supported(_get_rid(), p_language);
}

bool Font::is_script_supported(const String& p_script) const
{
	return TS->font_is_script_supported(_get_rid(), p_script);
}

int64_t Font::get_face_count() const { return TS->font_get_face_count(_get_rid()); }

Font::Font()
{
	cache.set_capacity(64);
	cache_wrap.set_capacity(16);
}

Font::~Font() {}

/*************************************************************************/
/*  FontFile                                                             */
/*************************************************************************/

_FORCE_INLINE_ void FontFile::_clear_cache()
{
	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			if (TS.is_valid()) {
				TS->free_rid(cache[i]);
			}
			cache.write[i] = RID();
		}
	}
}

void FontFile::_convert_packed_8bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_r;
	imgdata_r.resize(w * h * 2);
	uint8_t* wr = imgdata_r.ptrw();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_b;
	imgdata_b.resize(w * h * 2);
	uint8_t* wb = imgdata_b.ptrw();

	PackedByteArray imgdata_a;
	imgdata_a.resize(w * h * 2);
	uint8_t* wa = imgdata_a.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * 4;
			int ofs_dst = (i * w + j) * 2;
			wr[ofs_dst + 0] = 255;
			wr[ofs_dst + 1] = r[ofs_src + 0];
			wg[ofs_dst + 0] = 255;
			wg[ofs_dst + 1] = r[ofs_src + 1];
			wb[ofs_dst + 0] = 255;
			wb[ofs_dst + 1] = r[ofs_src + 2];
			wa[ofs_dst + 0] = 255;
			wa[ofs_dst + 1] = r[ofs_src + 3];
		}
	}
	Ref<Image> img_r = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_r));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 0, img_r);
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 1, img_g);
	Ref<Image> img_b = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_b));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 2, img_b);
	Ref<Image> img_a = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_a));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 3, img_a);
}

void FontFile::_convert_packed_4bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_r;
	imgdata_r.resize(w * h * 2);
	uint8_t* wr = imgdata_r.ptrw();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_b;
	imgdata_b.resize(w * h * 2);
	uint8_t* wb = imgdata_b.ptrw();

	PackedByteArray imgdata_a;
	imgdata_a.resize(w * h * 2);
	uint8_t* wa = imgdata_a.ptrw();

	PackedByteArray imgdata_ro;
	imgdata_ro.resize(w * h * 2);
	uint8_t* wro = imgdata_ro.ptrw();

	PackedByteArray imgdata_go;
	imgdata_go.resize(w * h * 2);
	uint8_t* wgo = imgdata_go.ptrw();

	PackedByteArray imgdata_bo;
	imgdata_bo.resize(w * h * 2);
	uint8_t* wbo = imgdata_bo.ptrw();

	PackedByteArray imgdata_ao;
	imgdata_ao.resize(w * h * 2);
	uint8_t* wao = imgdata_ao.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * 4;
			int ofs_dst = (i * w + j) * 2;
			wr[ofs_dst + 0] = 255;
			wro[ofs_dst + 0] = 255;
			if (r[ofs_src + 0] > 0x0F) {
				wr[ofs_dst + 1] = (r[ofs_src + 0] - 0x0F) * 2;
				wro[ofs_dst + 1] = 0;
			}
			else {
				wr[ofs_dst + 1] = 0;
				wro[ofs_dst + 1] = r[ofs_src + 0] * 2;
			}
			wg[ofs_dst + 0] = 255;
			wgo[ofs_dst + 0] = 255;
			if (r[ofs_src + 1] > 0x0F) {
				wg[ofs_dst + 1] = (r[ofs_src + 1] - 0x0F) * 2;
				wgo[ofs_dst + 1] = 0;
			}
			else {
				wg[ofs_dst + 1] = 0;
				wgo[ofs_dst + 1] = r[ofs_src + 1] * 2;
			}
			wb[ofs_dst + 0] = 255;
			wbo[ofs_dst + 0] = 255;
			if (r[ofs_src + 2] > 0x0F) {
				wb[ofs_dst + 1] = (r[ofs_src + 2] - 0x0F) * 2;
				wbo[ofs_dst + 1] = 0;
			}
			else {
				wb[ofs_dst + 1] = 0;
				wbo[ofs_dst + 1] = r[ofs_src + 2] * 2;
			}
			wa[ofs_dst + 0] = 255;
			wao[ofs_dst + 0] = 255;
			if (r[ofs_src + 3] > 0x0F) {
				wa[ofs_dst + 1] = (r[ofs_src + 3] - 0x0F) * 2;
				wao[ofs_dst + 1] = 0;
			}
			else {
				wa[ofs_dst + 1] = 0;
				wao[ofs_dst + 1] = r[ofs_src + 3] * 2;
			}
		}
	}
	Ref<Image> img_r = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_r));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 0, img_r);
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 1, img_g);
	Ref<Image> img_b = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_b));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 2, img_b);
	Ref<Image> img_a = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_a));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 3, img_a);

	Ref<Image> img_ro = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_ro));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 0, img_ro);
	Ref<Image> img_go = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_go));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 1, img_go);
	Ref<Image> img_bo = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_bo));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 2, img_bo);
	Ref<Image> img_ao = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_ao));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 3, img_ao);
}

void FontFile::_convert_rgba_4bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 4);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_o;
	imgdata_o.resize(w * h * 4);
	uint8_t* wo = imgdata_o.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs = (i * w + j) * 4;

			if (r[ofs + 0] > 0x7F) {
				wg[ofs + 0] = r[ofs + 0];
				wo[ofs + 0] = 0;
			}
			else {
				wg[ofs + 0] = 0;
				wo[ofs + 0] = r[ofs + 0] * 2;
			}
			if (r[ofs + 1] > 0x7F) {
				wg[ofs + 1] = r[ofs + 1];
				wo[ofs + 1] = 0;
			}
			else {
				wg[ofs + 1] = 0;
				wo[ofs + 1] = r[ofs + 1] * 2;
			}
			if (r[ofs + 2] > 0x7F) {
				wg[ofs + 2] = r[ofs + 2];
				wo[ofs + 2] = 0;
			}
			else {
				wg[ofs + 2] = 0;
				wo[ofs + 2] = r[ofs + 2] * 2;
			}
			if (r[ofs + 3] > 0x7F) {
				wg[ofs + 3] = r[ofs + 3];
				wo[ofs + 3] = 0;
			}
			else {
				wg[ofs + 3] = 0;
				wo[ofs + 3] = r[ofs + 3] * 2;
			}
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_RGBA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page, img_g);

	Ref<Image> img_o = memnew(Image(w, h, false, Image::FORMAT_RGBA8, imgdata_o));
	set_texture_image(0, Vector2i(p_sz, 1), p_page, img_o);
}

void FontFile::_convert_mono_8bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	int size = 4;
	if (p_source->get_format() == Image::FORMAT_L8) {
		size = 1;
		p_ch = 0;
	}

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * size;
			int ofs_dst = (i * w + j) * 2;
			wg[ofs_dst + 0] = 255;
			wg[ofs_dst + 1] = r[ofs_src + p_ch];
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, p_ol), p_page, img_g);
}

void FontFile::_convert_mono_4bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	int size = 4;
	if (p_source->get_format() == Image::FORMAT_L8) {
		size = 1;
		p_ch = 0;
	}

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_o;
	imgdata_o.resize(w * h * 2);
	uint8_t* wo = imgdata_o.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * size;
			int ofs_dst = (i * w + j) * 2;
			wg[ofs_dst + 0] = 255;
			wo[ofs_dst + 0] = 255;
			if (r[ofs_src + p_ch] > 0x7F) {
				wg[ofs_dst + 1] = r[ofs_src + p_ch];
				wo[ofs_dst + 1] = 0;
			}
			else {
				wg[ofs_dst + 1] = 0;
				wo[ofs_dst + 1] = r[ofs_src + p_ch] * 2;
			}
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page, img_g);

	Ref<Image> img_o = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_o));
	set_texture_image(0, Vector2i(p_sz, p_ol), p_page, img_o);
}

void FontFile::reset_state()
{
	_clear_cache();
	data.clear();
	data_ptr = nullptr;
	data_size = 0;
	cache.clear();

	antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	mipmaps = false;
	disable_embedded_bitmaps = true;
	msdf = false;
	force_autohinter = false;
	modulate_color_glyphs = false;
	allow_system_fallback = true;
	hinting = TextServer::HINTING_LIGHT;
	subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_DISABLED;
	keep_rounding_remainders = true;
	oversampling_override = 0.0;
	msdf_pixel_range = 14;
	msdf_size = 128;
	fixed_size = 0;
	fixed_size_scale_mode = TextServer::FIXED_SIZE_SCALE_DISABLE;

	Font::reset_state();
}

/*************************************************************************/

// OEM encoding mapping for 0x80..0xFF range.
static const char32_t _oem_to_unicode[][129] = {
	U"\u20ac\ufffe\u201a\ufffe\u201e\u2026\u2020\u2021\ufffe\u2030\u0160\u2039\u015a\u0164\u017d"
	U"\u0179\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\u0161\u203a\u015b\u0165"
	U"\u017e\u017a\xa0\u02c7\u02d8\u0141\xa4\u0104\xa6\xa7\xa8\xa9\u015e\xab\xac\xad\xae\u017b\xb0"
	U"\xb1\u02db\u0142\xb4\xb5\xb6\xb7\xb8\u0105\u015f\xbb\u013d\u02dd\u013e\u017c\u0154\xc1\xc2"
	U"\u0102\xc4\u0139\u0106\xc7\u010c\xc9\u0118\xcb\u011a\xcd\xce\u010e\u0110\u0143\u0147\xd3\xd4"
	U"\u0150\xd6\xd7\u0158\u016e\xda\u0170\xdc\xdd\u0162\xdf\u0155\xe1\xe2\u0103\xe4\u013a\u0107"
	U"\xe7\u010d\xe9\u0119\xeb\u011b\xed\xee\u010f\u0111\u0144\u0148\xf3\xf4\u0151\xf6\xf7\u0159"
	U"\u016f\xfa\u0171\xfc\xfd\u0163\u02d9", // 1250 - Latin 2
	U"\u0402\u0403\u201a\u0453\u201e\u2026\u2020\u2021\u20ac\u2030\u0409\u2039\u040a\u040c\u040b"
	U"\u040f\u0452\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\u0459\u203a\u045a\u045c"
	U"\u045b\u045f\xa0\u040e\u045e\u0408\xa4\u0490\xa6\xa7\u0401\xa9\u0404\xab\xac\xad\xae\u0407"
	U"\xb0\xb1\u0406\u0456\u0491\xb5\xb6\xb7\u0451\u2116\u0454\xbb\u0458\u0405\u0455\u0457\u0410"
	U"\u0411\u0412\u0413\u0414\u0415\u0416\u0417\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f"
	U"\u0420\u0421\u0422\u0423\u0424\u0425\u0426\u0427\u0428\u0429\u042a\u042b\u042c\u042d\u042e"
	U"\u042f\u0430\u0431\u0432\u0433\u0434\u0435\u0436\u0437\u0438\u0439\u043a\u043b\u043c\u043d"
	U"\u043e\u043f\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044a\u044b\u044c"
	U"\u044d\u044e\u044f", // 1251 - Cyrillic
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0160\u2039\u0152\ufffe\u017d"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\u0161\u203a\u0153\ufffe"
	U"\u017e\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca"
	U"\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1"
	U"\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8"
	U"\xf9\xfa\xfb\xfc\xfd\xfe\xff", // 1252 - Latin 1
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\ufffe\u2030\ufffe\u2039\ufffe\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\ufffe\u203a\ufffe\ufffe"
	U"\ufffe\ufffe\xa0\u0385\u0386\xa3\xa4\xa5\xa6\xa7\xa8\xa9\ufffe\xab\xac\xad\xae\u2015\xb0\xb1"
	U"\xb2\xb3\u0384\xb5\xb6\xb7\u0388\u0389\u038a\xbb\u038c\xbd\u038e\u038f\u0390\u0391\u0392"
	U"\u0393\u0394\u0395\u0396\u0397\u0398\u0399\u039a\u039b\u039c\u039d\u039e\u039f\u03a0\u03a1"
	U"\ufffe\u03a3\u03a4\u03a5\u03a6\u03a7\u03a8\u03a9\u03aa\u03ab\u03ac\u03ad\u03ae\u03af\u03b0"
	U"\u03b1\u03b2\u03b3\u03b4\u03b5\u03b6\u03b7\u03b8\u03b9\u03ba\u03bb\u03bc\u03bd\u03be\u03bf"
	U"\u03c0\u03c1\u03c2\u03c3\u03c4\u03c5\u03c6\u03c7\u03c8\u03c9\u03ca\u03cb\u03cc\u03cd\u03ce"
	U"\ufffe", // 1253 - Greek
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0160\u2039\u0152\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\u0161\u203a\u0153\ufffe"
	U"\ufffe\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca"
	U"\xcb\xcc\xcd\xce\xcf\u011e\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\u0130\u015e\xdf"
	U"\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\u011f\xf1\xf2\xf3\xf4\xf5"
	U"\xf6\xf7\xf8\xf9\xfa\xfb\xfc\u0131\u015f\xff", // 1254 - Turkish
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\ufffe\u2039\ufffe\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\ufffe\u203a\ufffe\ufffe"
	U"\ufffe\ufffe\xa0\xa1\xa2\xa3\u20aa\xa5\xa6\xa7\xa8\xa9\xd7\xab\xac\xad\xae\xaf\xb0\xb1\xb2"
	U"\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xf7\xbb\xbc\xbd\xbe\xbf\u05b0\u05b1\u05b2\u05b3\u05b4\u05b5"
	U"\u05b6\u05b7\u05b8\u05b9\ufffe\u05bb\u05bc\u05bd\u05be\u05bf\u05c0\u05c1\u05c2\u05c3\u05f0"
	U"\u05f1\u05f2\u05f3\u05f4\ufffe\ufffe\ufffe\ufffe\ufffe\ufffe\ufffe\u05d0\u05d1\u05d2\u05d3"
	U"\u05d4\u05d5\u05d6\u05d7\u05d8\u05d9\u05da\u05db\u05dc\u05dd\u05de\u05df\u05e0\u05e1\u05e2"
	U"\u05e3\u05e4\u05e5\u05e6\u05e7\u05e8\u05e9\u05ea\ufffe\ufffe\u200e\u200f\ufffe", // 1255 -
																					   // Hebrew
	U"\u20ac\u067e\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0679\u2039\u0152\u0686\u0698"
	U"\u0688\u06af\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u06a9\u2122\u0691\u203a\u0153\u200c"
	U"\u200d\u06ba\xa0\u060c\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\u06be\xab\xac\xad\xae\xaf\xb0\xb1\xb2"
	U"\xb3\xb4\xb5\xb6\xb7\xb8\xb9\u061b\xbb\xbc\xbd\xbe\u061f\u06c1\u0621\u0622\u0623\u0624\u0625"
	U"\u0626\u0627\u0628\u0629\u062a\u062b\u062c\u062d\u062e\u062f\u0630\u0631\u0632\u0633\u0634"
	U"\u0635\u0636\xd7\u0637\u0638\u0639\u063a\u0640\u0641\u0642\u0643\xe0\u0644\xe2\u0645\u0646"
	U"\u0647\u0648\xe7\xe8\xe9\xea\xeb\u0649\u064a\xee\xef\u064b\u064c\u064d\u064e\xf4\u064f\u0650"
	U"\xf7\u0651\xf9\u0652\xfb\xfc\u200e\u200f\u06d2", // 1256 - Arabic
	U"\u20ac\ufffe\u201a\ufffe\u201e\u2026\u2020\u2021\ufffe\u2030\ufffe\u2039\ufffe\xa8\u02c7\xb8"
	U"\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\ufffe\u203a\ufffe\xaf\u02db"
	U"\ufffe\xa0\ufffe\xa2\xa3\xa4\ufffe\xa6\xa7\xd8\xa9\u0156\xab\xac\xad\xae\xc6\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xf8\xb9\u0157\xbb\xbc\xbd\xbe\xe6\u0104\u012e\u0100\u0106\xc4\xc5\u0118"
	U"\u0112\u010c\xc9\u0179\u0116\u0122\u0136\u012a\u013b\u0160\u0143\u0145\xd3\u014c\xd5\xd6\xd7"
	U"\u0172\u0141\u015a\u016a\xdc\u017b\u017d\xdf\u0105\u012f\u0101\u0107\xe4\xe5\u0119\u0113"
	U"\u010d\xe9\u017a\u0117\u0123\u0137\u012b\u013c\u0161\u0144\u0146\xf3\u014d\xf5\xf6\xf7\u0173"
	U"\u0142\u015b\u016b\xfc\u017c\u017e\u02d9", // 1257 - Baltic
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\ufffe\u2039\u0152\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\ufffe\u203a\u0153\ufffe"
	U"\ufffe\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\u0102\xc4\xc5\xc6\xc7\xc8\xc9"
	U"\xca\xcb\u0300\xcd\xce\xcf\u0110\xd1\u0309\xd3\xd4\u01a0\xd6\xd7\xd8\xd9\xda\xdb\xdc\u01af"
	U"\u0303\xdf\xe0\xe1\xe2\u0103\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\u0301\xed\xee\xef\u0111\xf1"
	U"\u0323\xf3\xf4\u01a1\xf6\xf7\xf8\xf9\xfa\xfb\xfc\u01b0\u20ab\xff", // 1258 - Vietnamese
};

Error FontFile::load_bitmap_font(const String& p_path)
{
	return _load_bitmap_font(p_path, nullptr);
}

Error FontFile::load_dynamic_font(const String& p_path)
{
	reset_state();

	Vector<uint8_t> font_data = FileAccess::get_file_as_bytes(p_path);
	set_data(font_data);

	return OK;
}

void FontFile::set_data_ptr(const uint8_t* p_data, size_t p_size)
{
	data.clear();
	data_ptr = p_data;
	data_size = p_size;

	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			TS->font_set_data_ptr(cache[i], data_ptr, data_size);
		}
	}
}

void FontFile::set_data(const PackedByteArray& p_data)
{
	data = p_data;
	data_ptr = data.ptr();
	data_size = data.size();

	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			TS->font_set_data_ptr(cache[i], data_ptr, data_size);
		}
	}
}

PackedByteArray FontFile::get_data() const
{
	if (unlikely((size_t)data.size() != data_size)) {
		data.resize(data_size);
		memcpy(data.ptrw(), data_ptr, data_size);
	}
	return data;
}

TextServer::FontAntialiasing FontFile::get_antialiasing() const { return antialiasing; }

bool FontFile::get_disable_embedded_bitmaps() const { return disable_embedded_bitmaps; }

bool FontFile::get_generate_mipmaps() const { return mipmaps; }

bool FontFile::is_multichannel_signed_distance_field() const { return msdf; }

int FontFile::get_msdf_pixel_range() const { return msdf_pixel_range; }

int FontFile::get_msdf_size() const { return msdf_size; }

int FontFile::get_fixed_size() const { return fixed_size; }

TextServer::FixedSizeScaleMode FontFile::get_fixed_size_scale_mode() const
{
	return fixed_size_scale_mode;
}

bool FontFile::is_allow_system_fallback() const { return allow_system_fallback; }

bool FontFile::is_force_autohinter() const { return force_autohinter; }

bool FontFile::is_modulate_color_glyphs() const { return modulate_color_glyphs; }

TextServer::Hinting FontFile::get_hinting() const { return hinting; }

TextServer::SubpixelPositioning FontFile::get_subpixel_positioning() const
{
	return subpixel_positioning;
}

bool FontFile::get_keep_rounding_remainders() const { return keep_rounding_remainders; }

real_t FontFile::get_oversampling() const { return oversampling_override; }

int FontFile::get_cache_count() const { return cache.size(); }

void FontFile::clear_cache()
{
	_clear_cache();
	cache.clear();
	emit_changed();
}

void FontFile::remove_cache(int p_cache_index)
{
	ERR_FAIL_INDEX(p_cache_index, cache.size());
	if (cache[p_cache_index].is_valid()) {
		TS->free_rid(cache.write[p_cache_index]);
	}
	cache.remove_at(p_cache_index);
	emit_changed();
}

bool FontFile::get_language_support_override(const String& p_language) const
{
	if (language_support_overrides.has(p_language)) {
		return language_support_overrides[p_language];
	}
	else {
		return false;
	}
}

Vector<String> FontFile::get_language_support_overrides() const
{
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : language_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

bool FontFile::get_script_support_override(const String& p_script) const
{
	if (script_support_overrides.has(p_script)) {
		return script_support_overrides[p_script];
	}
	else {
		return false;
	}
}

Vector<String> FontFile::get_script_support_overrides() const
{
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : script_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

FontFile::FontFile() {}

FontFile::~FontFile() { _clear_cache(); }

Ref<Font> FontVariation::get_base_font() const { return base_font; }

void FontVariation::set_variation_embolden(float p_strength)
{
	if (variation.embolden != p_strength) {
		variation.embolden = p_strength;
		_invalidate_rids();
	}
}

float FontVariation::get_variation_embolden() const { return variation.embolden; }

void FontVariation::set_variation_transform(Transform2D p_transform)
{
	if (variation.transform != p_transform) {
		variation.transform = p_transform;
		_invalidate_rids();
	}
}

Transform2D FontVariation::get_variation_transform() const { return variation.transform; }

void FontVariation::set_variation_face_index(int p_face_index)
{
	if (variation.face_index != p_face_index) {
		variation.face_index = p_face_index;
		_invalidate_rids();
	}
}

int FontVariation::get_variation_face_index() const { return variation.face_index; }

void FontVariation::set_spacing(TextServer::SpacingType p_spacing, int p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, TextServer::SPACING_MAX);
	if (extra_spacing[p_spacing] != p_value) {
		extra_spacing[p_spacing] = p_value;
		_invalidate_rids();
	}
}

int FontVariation::get_spacing(TextServer::SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, TextServer::SPACING_MAX, 0);
	return extra_spacing[p_spacing];
}

void FontVariation::set_baseline_offset(float p_baseline_offset)
{
	if (baseline_offset != p_baseline_offset) {
		baseline_offset = p_baseline_offset;
		_invalidate_rids();
	}
}

float FontVariation::get_baseline_offset() const { return baseline_offset; }

void FontVariation::set_palette_index(int64_t p_palette_index)
{
	if (palette_index != p_palette_index) {
		palette_index = p_palette_index;
		_invalidate_rids();
	}
}

int64_t FontVariation::get_palette_index() const { return palette_index; }

void FontVariation::set_palette_custom_colors(const Vector<Color>& p_colors)
{
	if (custom_colors != p_colors) {
		custom_colors = p_colors;
		_invalidate_rids();
	}
}

Vector<Color> FontVariation::get_palette_custom_colors() const { return custom_colors; }

FontVariation::FontVariation()
{
	for (int i = 0; i < TextServer::SPACING_MAX; i++) {
		extra_spacing[i] = 0;
	}
}

FontVariation::~FontVariation() {}

/*************************************************************************/
/*  SystemFont                                                           */
/*************************************************************************/

void SystemFont::set_antialiasing(TextServer::FontAntialiasing p_antialiasing)
{
	if (antialiasing != p_antialiasing) {
		antialiasing = p_antialiasing;
		if (base_font.is_valid()) {
			base_font->set_antialiasing(antialiasing);
		}
		emit_changed();
	}
}

TextServer::FontAntialiasing SystemFont::get_antialiasing() const { return antialiasing; }

void SystemFont::set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps)
{
	if (disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		disable_embedded_bitmaps = p_disable_embedded_bitmaps;
		if (base_font.is_valid()) {
			base_font->set_disable_embedded_bitmaps(disable_embedded_bitmaps);
		}
		emit_changed();
	}
}

bool SystemFont::get_disable_embedded_bitmaps() const { return disable_embedded_bitmaps; }

void SystemFont::set_generate_mipmaps(bool p_generate_mipmaps)
{
	if (mipmaps != p_generate_mipmaps) {
		mipmaps = p_generate_mipmaps;
		if (base_font.is_valid()) {
			base_font->set_generate_mipmaps(mipmaps);
		}
		emit_changed();
	}
}

bool SystemFont::get_generate_mipmaps() const { return mipmaps; }

void SystemFont::set_allow_system_fallback(bool p_allow_system_fallback)
{
	if (allow_system_fallback != p_allow_system_fallback) {
		allow_system_fallback = p_allow_system_fallback;
		if (base_font.is_valid()) {
			base_font->set_allow_system_fallback(allow_system_fallback);
		}
		emit_changed();
	}
}

bool SystemFont::is_allow_system_fallback() const { return allow_system_fallback; }

void SystemFont::set_force_autohinter(bool p_force_autohinter)
{
	if (force_autohinter != p_force_autohinter) {
		force_autohinter = p_force_autohinter;
		if (base_font.is_valid()) {
			base_font->set_force_autohinter(force_autohinter);
		}
		emit_changed();
	}
}

bool SystemFont::is_force_autohinter() const { return force_autohinter; }

void SystemFont::set_modulate_color_glyphs(bool p_modulate)
{
	if (modulate_color_glyphs != p_modulate) {
		modulate_color_glyphs = p_modulate;
		if (base_font.is_valid()) {
			base_font->set_modulate_color_glyphs(modulate_color_glyphs);
		}
		emit_changed();
	}
}

bool SystemFont::is_modulate_color_glyphs() const { return modulate_color_glyphs; }

void SystemFont::set_hinting(TextServer::Hinting p_hinting)
{
	if (hinting != p_hinting) {
		hinting = p_hinting;
		if (base_font.is_valid()) {
			base_font->set_hinting(hinting);
		}
		emit_changed();
	}
}

TextServer::Hinting SystemFont::get_hinting() const { return hinting; }

void SystemFont::set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel)
{
	if (subpixel_positioning != p_subpixel) {
		subpixel_positioning = p_subpixel;
		if (base_font.is_valid()) {
			base_font->set_subpixel_positioning(subpixel_positioning);
		}
		emit_changed();
	}
}

TextServer::SubpixelPositioning SystemFont::get_subpixel_positioning() const
{
	return subpixel_positioning;
}

void SystemFont::set_keep_rounding_remainders(bool p_keep_rounding_remainders)
{
	if (keep_rounding_remainders != p_keep_rounding_remainders) {
		keep_rounding_remainders = p_keep_rounding_remainders;
		if (base_font.is_valid()) {
			base_font->set_keep_rounding_remainders(keep_rounding_remainders);
		}
		emit_changed();
	}
}

bool SystemFont::get_keep_rounding_remainders() const { return keep_rounding_remainders; }

void SystemFont::set_oversampling(real_t p_oversampling)
{
	if (oversampling_override != p_oversampling) {
		oversampling_override = p_oversampling;
		if (base_font.is_valid()) {
			base_font->set_oversampling(oversampling_override);
		}
		emit_changed();
	}
}

real_t SystemFont::get_oversampling() const { return oversampling_override; }

void SystemFont::set_multichannel_signed_distance_field(bool p_msdf)
{
	if (msdf != p_msdf) {
		msdf = p_msdf;
		if (base_font.is_valid()) {
			base_font->set_multichannel_signed_distance_field(msdf);
		}
		emit_changed();
	}
}

bool SystemFont::is_multichannel_signed_distance_field() const { return msdf; }

void SystemFont::set_msdf_pixel_range(int p_msdf_pixel_range)
{
	if (msdf_pixel_range != p_msdf_pixel_range) {
		msdf_pixel_range = p_msdf_pixel_range;
		if (base_font.is_valid()) {
			base_font->set_msdf_pixel_range(msdf_pixel_range);
		}
		emit_changed();
	}
}

int SystemFont::get_msdf_pixel_range() const { return msdf_pixel_range; }

void SystemFont::set_msdf_size(int p_msdf_size)
{
	if (msdf_size != p_msdf_size) {
		msdf_size = p_msdf_size;
		if (base_font.is_valid()) {
			base_font->set_msdf_size(msdf_size);
		}
		emit_changed();
	}
}

int SystemFont::get_msdf_size() const { return msdf_size; }

void SystemFont::set_font_names(const PackedStringArray& p_names)
{
	if (names != p_names) {
		names = p_names;
		_update_base_font();
	}
}

PackedStringArray SystemFont::get_font_names() const { return names; }

void SystemFont::set_font_italic(bool p_italic)
{
	if (italic != p_italic) {
		italic = p_italic;
		_update_base_font();
	}
}

bool SystemFont::get_font_italic() const { return italic; }

void SystemFont::set_font_weight(int p_weight)
{
	if (weight != p_weight) {
		weight = CLAMP(p_weight, 100, 999);
		_update_base_font();
	}
}

int SystemFont::get_font_weight() const { return weight; }

void SystemFont::set_font_stretch(int p_stretch)
{
	if (stretch != p_stretch) {
		stretch = CLAMP(p_stretch, 50, 200);
		_update_base_font();
	}
}

int SystemFont::get_font_stretch() const { return stretch; }

int SystemFont::get_spacing(TextServer::SpacingType p_spacing) const
{
	if (base_font.is_valid()) {
		return base_font->get_spacing(p_spacing);
	}
	else {
		return 0;
	}
}

int64_t SystemFont::get_face_count() const { return face_indices.size(); }

void SystemFont::_update_base_font() {}

SystemFont::SystemFont()
{ /* NOP */
}

SystemFont::~SystemFont() {}

void FontVariation::_update_rids() const {}

void FontVariation::reset_state() {}

Error FontFile::_load_bitmap_font(String const&, List<String, DefaultAllocator>*) {}

void FontFile::set_transform(int p_cache_index, Transform2D p_transform) {}

void FontFile::set_texture_offsets(int p_cache_index, const Vector2i& p_size, int p_texture_index,
	const PackedInt32Array& p_offset)
{
}

void FontFile::set_texture_image(
	int p_cache_index, const Vector2i& p_size, int p_texture_index, const Ref<Image>& p_image)
{
}

void FontFile::set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel)
{
	if (subpixel_positioning != p_subpixel) {
		subpixel_positioning = p_subpixel;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_subpixel_positioning(cache[i], subpixel_positioning);
			}
		}
	}
}

void FontFile::set_script_support_override(const String& p_script, bool p_supported) {}

void FontFile::set_oversampling(real_t p_oversampling)
{
	if (oversampling_override != p_oversampling) {
		oversampling_override = p_oversampling;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_oversampling(cache[i], oversampling_override);
			}
		}
	}
}

void FontFile::set_multichannel_signed_distance_field(bool p_msdf)
{
	if (msdf != p_msdf) {
		msdf = p_msdf;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_multichannel_signed_distance_field(cache[i], msdf);
			}
		}
	}
}

void FontFile::set_msdf_size(int p_msdf_size)
{
	if (msdf_size != p_msdf_size) {
		msdf_size = p_msdf_size;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_msdf_size(cache[i], msdf_size);
			}
		}
	}
}

void FontFile::set_msdf_pixel_range(int p_msdf_pixel_range)
{
	if (msdf_pixel_range != p_msdf_pixel_range) {
		msdf_pixel_range = p_msdf_pixel_range;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_msdf_pixel_range(cache[i], msdf_pixel_range);
			}
		}
	}
}

void FontFile::set_modulate_color_glyphs(bool p_modulate)
{
	if (modulate_color_glyphs != p_modulate) {
		modulate_color_glyphs = p_modulate;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_modulate_color_glyphs(cache[i], modulate_color_glyphs);
			}
		}
	}
}

void FontFile::set_language_support_override(const String& p_language, bool p_supported) {}

void FontFile::set_kerning(
	int p_cache_index, int p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning)
{
}

void FontFile::set_keep_rounding_remainders(bool p_keep_rounding_remainders)
{
	if (keep_rounding_remainders != p_keep_rounding_remainders) {
		keep_rounding_remainders = p_keep_rounding_remainders;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_keep_rounding_remainders(cache[i], keep_rounding_remainders);
			}
		}
	}
}

void FontFile::set_hinting(TextServer::Hinting p_hinting)
{
	if (hinting != p_hinting) {
		hinting = p_hinting;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_hinting(cache[i], hinting);
			}
		}
	}
}

void FontFile::set_glyph_uv_rect(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Rect2& p_uv_rect)
{
}

void FontFile::set_glyph_texture_idx(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, int p_texture_idx)
{
}

void FontFile::set_glyph_size(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_gl_size)
{
}

void FontFile::set_glyph_offset(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_offset)
{
}

void FontFile::set_glyph_advance(
	int p_cache_index, int p_size, int32_t p_glyph, const Vector2& p_advance)
{
}

void FontFile::set_generate_mipmaps(bool p_generate_mipmaps)
{
	if (mipmaps != p_generate_mipmaps) {
		mipmaps = p_generate_mipmaps;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_generate_mipmaps(cache[i], mipmaps);
			}
		}
	}
}

void FontFile::set_force_autohinter(bool p_force_autohinter)
{
	if (force_autohinter != p_force_autohinter) {
		force_autohinter = p_force_autohinter;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_force_autohinter(cache[i], force_autohinter);
			}
		}
	}
}

void FontFile::set_font_weight(int p_weight) {}

void FontFile::set_font_style(uint32_t p_style) {}

void FontFile::set_font_style_name(const String& p_name) {}

void FontFile::set_font_stretch(int p_stretch) {}

void FontFile::set_font_name(const String& p_name) {}

void FontFile::set_fixed_size_scale_mode(TextServer::FixedSizeScaleMode p_fixed_size_scale_mode)
{
	if (fixed_size_scale_mode != p_fixed_size_scale_mode) {
		fixed_size_scale_mode = p_fixed_size_scale_mode;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_fixed_size_scale_mode(cache[i], fixed_size_scale_mode);
			}
		}
	}
}

void FontFile::set_fixed_size(int p_fixed_size)
{
	if (fixed_size != p_fixed_size) {
		fixed_size = p_fixed_size;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_fixed_size(cache[i], fixed_size);
			}
		}
	}
}

void FontFile::set_face_index(int p_cache_index, int64_t p_index) {}

void FontFile::set_extra_spacing(
	int p_cache_index, TextServer::SpacingType p_spacing, int64_t p_value)
{
}

void FontFile::set_extra_baseline_offset(int p_cache_index, float p_baseline_offset) {}

void FontFile::set_embolden(int p_cache_index, float p_strength) {}

void FontFile::set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps)
{
	if (disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		disable_embedded_bitmaps = p_disable_embedded_bitmaps;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_disable_embedded_bitmaps(cache[i], disable_embedded_bitmaps);
			}
		}
	}
}

void FontFile::set_cache_underline_thickness(
	int p_cache_index, int p_size, real_t p_underline_thickness)
{
}

void FontFile::set_cache_underline_position(
	int p_cache_index, int p_size, real_t p_underline_position)
{
}

void FontFile::set_cache_scale(int p_cache_index, int p_size, real_t p_scale) {}

void FontFile::set_cache_descent(int p_cache_index, int p_size, real_t p_descent) {}

void FontFile::set_cache_ascent(int p_cache_index, int p_size, real_t p_ascent) {}

void FontFile::set_antialiasing(TextServer::FontAntialiasing p_antialiasing)
{
	if (antialiasing != p_antialiasing) {
		antialiasing = p_antialiasing;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_antialiasing(cache[i], antialiasing);
			}
		}
	}
}

void FontFile::set_allow_system_fallback(bool p_allow_system_fallback)
{
	if (allow_system_fallback != p_allow_system_fallback) {
		allow_system_fallback = p_allow_system_fallback;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_allow_system_fallback(cache[i], allow_system_fallback);
			}
		}
	}
}

void FontFile::render_range(
	int p_cache_index, const Vector2i& p_size, char32_t p_start, char32_t p_end)
{
}

void FontFile::render_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_index) {}

void FontFile::remove_texture(int p_cache_index, const Vector2i& p_size, int p_texture_index) {}

void FontFile::remove_size_cache(int p_cache_index, const Vector2i& p_size) {}

void FontFile::remove_script_support_override(const String& p_script) {}

void FontFile::remove_language_support_override(const String& p_language) {}

void FontFile::remove_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) {}

void FontFile::remove_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) {}

Transform2D FontFile::get_transform(int p_cache_index) const { return Transform2D(); }

PackedInt32Array FontFile::get_texture_offsets(
	int p_cache_index, const Vector2i& p_size, int p_texture_index) const
{
	return PackedInt32Array();
}

Ref<Image> FontFile::get_texture_image(
	int p_cache_index, const Vector2i& p_size, int p_texture_index) const
{
	Ref<Image> i = memnew(Image);
	return i;
}

int FontFile::get_texture_count(int p_cache_index, const Vector2i& p_size) const { return 0; }

Vector2 FontFile::get_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) const
{
	return Vector2();
}

Rect2 FontFile::get_glyph_uv_rect(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Rect2();
}

int FontFile::get_glyph_texture_idx(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return 0;
}

Vector2 FontFile::get_glyph_size(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Vector2();
}

Vector2 FontFile::get_glyph_offset(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Vector2();
}

PackedInt32Array FontFile::get_glyph_list(int p_cache_index, const Vector2i& p_size) const
{
	return PackedInt32Array();
}

int32_t FontFile::get_glyph_index(int p_size, char32_t p_char, char32_t p_variation_selector) const
{
	return 0;
}

Vector2 FontFile::get_glyph_advance(int p_cache_index, int p_size, int32_t p_glyph) const
{
	return Vector2();
}

int64_t FontFile::get_face_index(int p_cache_index) const { return 0; }

int64_t FontFile::get_extra_spacing(int p_cache_index, TextServer::SpacingType p_spacing) const
{
	return 0;
}

float FontFile::get_extra_baseline_offset(int p_cache_index) const { return 0.0; }

float FontFile::get_embolden(int p_cache_index) const { return 0.0; }

char32_t FontFile::get_char_from_glyph_index(int p_size, int32_t p_glyph_index) const
{
	return ' ';
}

real_t FontFile::get_cache_underline_thickness(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_underline_position(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_scale(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_descent(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_ascent(int p_cache_index, int p_size) const { return 0.0; }

void FontFile::clear_textures(int p_cache_index, const Vector2i& p_size) {}

void FontFile::clear_size_cache(int p_cache_index) {}

void FontFile::clear_kerning_map(int p_cache_index, int p_size) {}

void FontFile::clear_glyphs(int p_cache_index, const Vector2i& p_size) {}

RID FontFile::_get_rid() const
{
	if (TS.is_null() || data_ptr == nullptr || data_size == 0) {
		return RID();
	}
	if (unlikely(cache.is_empty())) {
		cache.resize(1);
	}
	if (unlikely(!cache[0].is_valid())) {
		RID rid = TS->create_font();
		if (rid.is_valid()) {
			TS->font_set_data_ptr(rid, data_ptr, data_size);
			TS->font_set_antialiasing(rid, antialiasing);
			TS->font_set_generate_mipmaps(rid, mipmaps);
			TS->font_set_disable_embedded_bitmaps(rid, disable_embedded_bitmaps);
			TS->font_set_multichannel_signed_distance_field(rid, msdf);
			TS->font_set_msdf_pixel_range(rid, msdf_pixel_range);
			TS->font_set_msdf_size(rid, msdf_size);
			TS->font_set_fixed_size(rid, fixed_size);
			TS->font_set_fixed_size_scale_mode(rid, fixed_size_scale_mode);
			TS->font_set_force_autohinter(rid, force_autohinter);
			TS->font_set_modulate_color_glyphs(rid, modulate_color_glyphs);
			TS->font_set_allow_system_fallback(rid, allow_system_fallback);
			TS->font_set_hinting(rid, hinting);
			TS->font_set_subpixel_positioning(rid, subpixel_positioning);
			TS->font_set_keep_rounding_remainders(rid, keep_rounding_remainders);
			TS->font_set_oversampling(rid, oversampling_override);
			cache.write[0] = rid;
		}
	}
	return cache[0];
}

void SystemFont::reset_state() {}

void SystemFont::_update_rids() const {}

RID SystemFont::_get_rid() const
{
	return base_font.is_valid() ? base_font->_get_rid() : RID();
}

Ref<Font> SystemFont::_get_base_font_or_default() const
{
	if (base_font.is_valid()) {
		return base_font;
	}
	return ThemeDB::get_fallback_font();
}

RID FontVariation::_get_rid() const
{
	Ref<Font> f = _get_base_font_or_default();
	return f.is_valid() ? f->_get_rid() : RID();
}

void FontVariation::set_base_font(const Ref<Font>& p_font)
{
	ERR_FAIL_COND_MSG(_is_base_cyclic(p_font, 0), "Cyclic font reference.");
	base_font = p_font;
	_invalidate_rids();
}

Ref<Font> FontVariation::_get_base_font_or_default() const
{
	if (base_font.is_valid()) {
		return base_font;
	}
	return ThemeDB::get_fallback_font();
}

// --- font.compat.inc ---

#ifndef DISABLE_DEPRECATED
real_t Font::_draw_char_bind_compat_104872(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
	int p_font_size, const Color& p_modulate) const
{
	return draw_char(p_canvas_item, p_pos, p_char, p_font_size, p_modulate, 0.0);
}

real_t Font::_draw_char_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
	char32_t p_char, int p_font_size, int p_size, const Color& p_modulate) const
{
	return draw_char_outline(p_canvas_item, p_pos, p_char, p_font_size, p_size, p_modulate, 0.0);
}

#endif // DISABLE_DEPRECATED

// --- text_line.cpp ---

#include "text_line.h"

RID TextLine::get_rid() const { return rid; }

void TextLine::clear() { TS->shaped_text_clear(rid); }

Ref<TextLine> TextLine::duplicate() const
{
	Ref<TextLine> copy;
	copy.instantiate();
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->dirty = true;
	copy->width = width;
	copy->flags = flags;
	copy->alignment = alignment;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextLine::set_preserve_invalid(bool p_enabled)
{
	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_invalid() const { return TS->shaped_text_get_preserve_invalid(rid); }

void TextLine::set_preserve_control(bool p_enabled)
{
	TS->shaped_text_set_preserve_control(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_control() const { return TS->shaped_text_get_preserve_control(rid); }

void TextLine::set_direction(TextServer::Direction p_direction)
{
	TS->shaped_text_set_direction(rid, p_direction);
	dirty = true;
}

TextServer::Direction TextLine::get_direction() const { return TS->shaped_text_get_direction(rid); }

TextServer::Direction TextLine::get_inferred_direction() const
{
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextLine::set_orientation(TextServer::Orientation p_orientation)
{
	TS->shaped_text_set_orientation(rid, p_orientation);
	dirty = true;
}

TextServer::Orientation TextLine::get_orientation() const
{
	return TS->shaped_text_get_orientation(rid);
}

void TextLine::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextLine::get_horizontal_alignment() const { return alignment; }

void TextLine::tab_align(const Vector<float>& p_tab_stops)
{
	tab_stops = p_tab_stops;
	dirty = true;
}

void TextLine::set_flags(uint32_t p_flags)
{
	if (flags != p_flags) {
		flags = p_flags;
		dirty = true;
	}
}

uint32_t TextLine::get_flags() const { return flags; }

void TextLine::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		dirty = true;
	}
}

TextServer::OverrunBehavior TextLine::get_text_overrun_behavior() const { return overrun_behavior; }

void TextLine::set_ellipsis_char(const String& p_char)
{
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}
	if (el_char == c) {
		return;
	}
	el_char = c;
	dirty = true;
}

String TextLine::get_ellipsis_char() const { return el_char; }

void TextLine::set_width(float p_width)
{
	if (width == p_width) {
		return;
	}
	width = p_width;
	if (alignment == HORIZONTAL_ALIGNMENT_FILL ||
		overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		dirty = true;
	}
}

float TextLine::get_width() const { return width; }

Size2 TextLine::get_size() const
{
	_shape();
	return TS->shaped_text_get_size(rid);
}

float TextLine::get_line_ascent() const
{
	_shape();
	return TS->shaped_text_get_ascent(rid);
}

float TextLine::get_line_descent() const
{
	_shape();
	return TS->shaped_text_get_descent(rid);
}

float TextLine::get_line_width() const
{
	_shape();
	return TS->shaped_text_get_width(rid);
}

float TextLine::get_line_underline_position() const
{
	_shape();
	return TS->shaped_text_get_underline_position(rid);
}

float TextLine::get_line_underline_thickness() const
{
	_shape();
	return TS->shaped_text_get_underline_thickness(rid);
}

void TextLine::draw(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_color, p_oversampling);
}

void TextLine::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw_outline(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_outline_size, p_color, p_oversampling);
}

int TextLine::hit_test(float p_coords) const
{
	_shape();

	return TS->shaped_text_hit_test_position(rid, p_coords);
}

TextLine::TextLine() { rid = TS->create_shaped_text(); }

TextLine::~TextLine() { TS->free_rid(rid); }

void TextLine::_shape() const {}


TextLine::TextLine(String const&, Ref<Font> const&, int, String const&, TextServer::Direction, TextServer::Orientation) {}

// --- text_line.compat.inc ---

#ifndef DISABLE_DEPRECATED
void TextLine::_draw_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color) const
{
	draw(p_canvas, p_pos, p_color, 0.0);
}

void TextLine::_draw_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_outline_size, const Color& p_color) const
{
	draw_outline(p_canvas, p_pos, p_outline_size, p_color, 0.0);
}

void TextLine::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- text_paragraph.cpp ---

#include "text_paragraph.h"

RID TextParagraph::get_rid() const { return rid; }

RID TextParagraph::get_line_rid(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), RID());
	return lines_rid[p_line];
}

RID TextParagraph::get_dropcap_rid() const { return dropcap_rid; }

void TextParagraph::clear()
{
	_THREAD_SAFE_METHOD_

	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->shaped_text_clear(rid);
	TS->shaped_text_clear(dropcap_rid);
}

Ref<TextParagraph> TextParagraph::duplicate() const
{
	Ref<TextParagraph> copy;
	copy.instantiate();
	if (dropcap_rid.is_valid()) {
		TS->free_rid(copy->dropcap_rid);
		copy->dropcap_rid = TS->shaped_text_duplicate(dropcap_rid);
	}
	copy->dropcap_lines = dropcap_lines;
	copy->dropcap_margins = dropcap_margins;
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->lines_dirty = true;
	copy->line_spacing = line_spacing;
	copy->width = width;
	copy->max_lines_visible = max_lines_visible;
	copy->brk_flags = brk_flags;
	copy->jst_flags = jst_flags;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->alignment = alignment;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextParagraph::set_preserve_invalid(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	TS->shaped_text_set_preserve_invalid(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_invalid() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_invalid(rid);
}

void TextParagraph::set_preserve_control(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_control(rid, p_enabled);
	TS->shaped_text_set_preserve_control(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_control() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_control(rid);
}

void TextParagraph::set_direction(TextServer::Direction p_direction)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_direction(rid, p_direction);
	TS->shaped_text_set_direction(dropcap_rid, p_direction);
	lines_dirty = true;
}

TextServer::Direction TextParagraph::get_direction() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_direction(rid);
}

TextServer::Direction TextParagraph::get_inferred_direction() const
{
	_THREAD_SAFE_METHOD_

	const_cast<TextParagraph*>(this)->_shape_lines();
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextParagraph::set_custom_punctuation(const String& p_punct)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_custom_punctuation(rid, p_punct);
	lines_dirty = true;
}

String TextParagraph::get_custom_punctuation() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_custom_punctuation(rid);
}

void TextParagraph::set_orientation(TextServer::Orientation p_orientation)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_orientation(rid, p_orientation);
	TS->shaped_text_set_orientation(dropcap_rid, p_orientation);
	lines_dirty = true;
}

TextServer::Orientation TextParagraph::get_orientation() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_orientation(rid);
}

void TextParagraph::clear_dropcap()
{
	_THREAD_SAFE_METHOD_
	dropcap_margins = Rect2();
	TS->shaped_text_clear(dropcap_rid);
	lines_dirty = true;
}

void TextParagraph::set_alignment(HorizontalAlignment p_alignment)
{
	_THREAD_SAFE_METHOD_

	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			lines_dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextParagraph::get_alignment() const { return alignment; }

void TextParagraph::tab_align(const Vector<float>& p_tab_stops)
{
	_THREAD_SAFE_METHOD_

	tab_stops = p_tab_stops;
	lines_dirty = true;
}

void TextParagraph::set_justification_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_justification_flags() const { return jst_flags; }

void TextParagraph::set_break_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (brk_flags != p_flags) {
		brk_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_break_flags() const { return brk_flags; }

void TextParagraph::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	_THREAD_SAFE_METHOD_

	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		lines_dirty = true;
	}
}

TextServer::OverrunBehavior TextParagraph::get_text_overrun_behavior() const
{
	return overrun_behavior;
}

void TextParagraph::set_ellipsis_char(const String& p_char)
{
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}
	if (el_char == c) {
		return;
	}
	el_char = c;
	lines_dirty = true;
}

String TextParagraph::get_ellipsis_char() const { return el_char; }

void TextParagraph::set_width(float p_width)
{
	_THREAD_SAFE_METHOD_

	if (width != p_width) {
		width = p_width;
		lines_dirty = true;
	}
}

float TextParagraph::get_width() const { return width; }

Size2 TextParagraph::get_non_wrapped_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_size(rid);
}

Size2 TextParagraph::get_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();

	float h_offset = 0.f;
	float v_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
		v_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
		v_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}

	Size2 size;
	int visible_lines = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();
	for (int i = 0; i < visible_lines; i++) {
		Size2 lsize = TS->shaped_text_get_size(lines_rid[i]);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.x += h_offset;
			}
			size.x = MAX(size.x, lsize.x);
			size.y += lsize.y;
			if (i != visible_lines - 1) {
				size.y += line_spacing;
			}
		}
		else {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.y += h_offset;
			}
			size.x += lsize.x;
			size.y = MAX(size.y, lsize.y);
			if (i != visible_lines - 1) {
				size.x += line_spacing;
			}
		}
	}
	if (h_offset > 0) {
		if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			size.y = MAX(size.y, v_offset);
		}
		else {
			size.x = MAX(size.x, v_offset);
		}
	}
	return size;
}

Vector2i TextParagraph::get_range() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_range(rid);
}

int TextParagraph::get_line_count() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return (int)lines_rid.size();
}

void TextParagraph::set_max_lines_visible(int p_lines)
{
	_THREAD_SAFE_METHOD_

	if (p_lines != max_lines_visible) {
		max_lines_visible = p_lines;
		lines_dirty = true;
	}
}

int TextParagraph::get_max_lines_visible() const { return max_lines_visible; }

void TextParagraph::set_line_spacing(float p_spacing)
{
	_THREAD_SAFE_METHOD_

	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		lines_dirty = true;
	}
}

float TextParagraph::get_line_spacing() const { return line_spacing; }

Size2 TextParagraph::get_line_size(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Size2());
	return TS->shaped_text_get_size(lines_rid[p_line]);
}

Vector2i TextParagraph::get_line_range(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Vector2i());
	return TS->shaped_text_get_range(lines_rid[p_line]);
}

float TextParagraph::get_line_ascent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_ascent(lines_rid[p_line]);
}

float TextParagraph::get_line_descent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_descent(lines_rid[p_line]);
}

float TextParagraph::get_line_width(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_width(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_position(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_position(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_thickness(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_thickness(lines_rid[p_line]);
}

Size2 TextParagraph::get_dropcap_size() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_size(dropcap_rid) + dropcap_margins.size + dropcap_margins.position;
}

int TextParagraph::get_dropcap_lines() const { return dropcap_lines; }

void TextParagraph::draw(RID p_canvas, const Vector2& p_pos, const Color& p_color,
	const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			dc_off + Vector2(0, TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.size.y +
									dropcap_margins.position.y / 2),
			-1, -1, p_dc_color, p_oversampling);
	}

	int lines_visible = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();

	for (int i = 0; i < lines_visible; i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float line_width = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (line_width <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - line_width) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - line_width) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - line_width;
				}
				else {
					ofs.y += l_width - line_width;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw(
			lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

void TextParagraph::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;

	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			dc_off + Vector2(dropcap_margins.position.x,
						 TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_dc_color, p_oversampling);
	}

	for (int i = 0; i < (int)lines_rid.size(); i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float length = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (length <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - length) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - length) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - length;
				}
				else {
					ofs.y += l_width - length;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw_outline(lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width,
			p_outline_size, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

int TextParagraph::hit_test(const Point2& p_coords) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		if (ofs.y < 0) {
			return 0;
		}
	}
	else {
		if (ofs.x < 0) {
			return 0;
		}
	}
	for (const RID& line_rid : lines_rid) {
		if (TS->shaped_text_get_orientation(line_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			if ((p_coords.y >= ofs.y) &&
				(p_coords.y <= ofs.y + TS->shaped_text_get_size(line_rid).y)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.x);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).y + line_spacing;
		}
		else {
			if ((p_coords.x >= ofs.x) &&
				(p_coords.x <= ofs.x + TS->shaped_text_get_size(line_rid).x)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.y);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).x + line_spacing;
		}
	}
	return TS->shaped_text_get_range(rid).y;
}

bool TextParagraph::is_dirty() { return lines_dirty; }

void TextParagraph::draw_dropcap(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_color, p_oversampling);
	}
}

void TextParagraph::draw_dropcap_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_color, p_oversampling);
	}
}

void TextParagraph::draw_line(RID p_canvas, const Vector2& p_pos, int p_line, const Color& p_color,
	float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;

	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw(lines_rid[p_line], p_canvas, ofs, -1, -1, p_color, p_oversampling);
}

void TextParagraph::draw_line_outline(RID p_canvas, const Vector2& p_pos, int p_line,
	int p_outline_size, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;
	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw_outline(
		lines_rid[p_line], p_canvas, ofs, -1, -1, p_outline_size, p_color, p_oversampling);
}

TextParagraph::TextParagraph()
{
	rid = TS->create_shaped_text();
	dropcap_rid = TS->create_shaped_text();
}

TextParagraph::~TextParagraph()
{
	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->free_rid(rid);
	TS->free_rid(dropcap_rid);
}

void TextParagraph::_shape_lines() const {}

// --- text_paragraph.compat.inc ---

#ifndef DISABLE_DEPRECATED
void TextParagraph::_draw_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, const Color& p_dc_color) const
{
	draw(p_canvas, p_pos, p_color, p_dc_color, 0.0);
}

void TextParagraph::_draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
	int p_outline_size, const Color& p_color, const Color& p_dc_color) const
{
	draw_outline(p_canvas, p_pos, p_outline_size, p_color, p_dc_color, 0.0);
}

void TextParagraph::_draw_line_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_line, const Color& p_color) const
{
	draw_line(p_canvas, p_pos, p_line, p_color, 0.0);
}

void TextParagraph::_draw_line_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_line, int p_outline_size, const Color& p_color) const
{
	draw_line_outline(p_canvas, p_pos, p_line, p_outline_size, p_color, 0.0);
}

void TextParagraph::_draw_dropcap_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color) const
{
	draw_dropcap(p_canvas, p_pos, p_color, 0.0);
}

void TextParagraph::_draw_dropcap_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_outline_size, const Color& p_color) const
{
	draw_dropcap_outline(p_canvas, p_pos, p_outline_size, p_color, 0.0);
}

void TextParagraph::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- label_settings.cpp ---

#include "scene/gui/label.h"

void LabelSettings::_font_changed() { emit_changed(); }

void LabelSettings::set_line_spacing(real_t p_spacing)
{
	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_line_spacing() const { return line_spacing; }

void LabelSettings::set_paragraph_spacing(real_t p_spacing)
{
	if (paragraph_spacing != p_spacing) {
		paragraph_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_paragraph_spacing() const { return paragraph_spacing; }

Ref<Font> LabelSettings::get_font() const { return font; }

void LabelSettings::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_font_size() const { return font_size; }

void LabelSettings::set_font_color(const Color& p_color)
{
	if (font_color != p_color) {
		font_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_font_color() const { return font_color; }

void LabelSettings::set_outline_size(int p_size)
{
	if (outline_size != p_size) {
		outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_outline_size() const { return outline_size; }

void LabelSettings::set_outline_color(const Color& p_color)
{
	if (outline_color != p_color) {
		outline_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_outline_color() const { return outline_color; }

void LabelSettings::set_shadow_size(int p_size)
{
	if (shadow_size != p_size) {
		shadow_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_shadow_size() const { return shadow_size; }

void LabelSettings::set_shadow_color(const Color& p_color)
{
	if (shadow_color != p_color) {
		shadow_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_shadow_color() const { return shadow_color; }

void LabelSettings::set_shadow_offset(const Vector2& p_offset)
{
	if (shadow_offset != p_offset) {
		shadow_offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_shadow_offset() const { return shadow_offset; }

Vector<LabelSettings::StackedOutlineData> LabelSettings::get_stacked_outline_data() const
{
	return stacked_outline_data;
}

int LabelSettings::get_stacked_outline_count() const { return stacked_outline_data.size(); }

void LabelSettings::set_stacked_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].size != p_size) {
		stacked_outline_data.write[p_index].size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), 0);
	return stacked_outline_data[p_index].size;
}

void LabelSettings::set_stacked_outline_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].color != p_color) {
		stacked_outline_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_outline_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), Color());
	return stacked_outline_data[p_index].color;
}

Vector<LabelSettings::StackedShadowData> LabelSettings::get_stacked_shadow_data() const
{
	return stacked_shadow_data;
}

int LabelSettings::get_stacked_shadow_count() const { return stacked_shadow_data.size(); }

void LabelSettings::set_stacked_shadow_offset(int p_index, const Vector2& p_offset)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].offset != p_offset) {
		stacked_shadow_data.write[p_index].offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_stacked_shadow_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Vector2());
	return stacked_shadow_data[p_index].offset;
}

void LabelSettings::set_stacked_shadow_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].color != p_color) {
		stacked_shadow_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_shadow_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Color());
	return stacked_shadow_data[p_index].color;
}

void LabelSettings::set_stacked_shadow_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].outline_size != p_size) {
		stacked_shadow_data.write[p_index].outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_shadow_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), 0);
	return stacked_shadow_data[p_index].outline_size;
}

// --- text_file.cpp ---

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
// --- Inlined from scene/resources/text_file.h ---
#include "core/io/resource.h"

class TextFile : public Resource
{
private:
	String text;
	String path;

protected:
	virtual bool editor_can_reload_from_file() override { return false; }

public:
	virtual bool has_text() const;
	virtual String get_text() const;
	virtual void set_text(const String& p_code);
	virtual void reload_from_file() override;

	void set_file_path(const String& p_path) { path = p_path; }

	Error load_text(const String& p_path);
};



// --- End scene/resources/text_file.h ---

bool TextFile::has_text() const { return !text.is_empty(); }

String TextFile::get_text() const { return text; }

void TextFile::set_text(const String& p_code) { text = p_code; }

void TextFile::reload_from_file() { load_text(path); }



Error TextFile::load_text(String const&) {}

// --- syntax_highlighter.cpp ---

#include "scene/gui/text_edit.h"

void CodeHighlighter::_clear_highlighting_cache() { color_region_cache.clear(); }

void CodeHighlighter::clear_highlighting_cache() { _clear_highlighting_cache(); }

void CodeHighlighter::_update_cache() { font_color = text_edit->get_font_color(); }

void CodeHighlighter::update_cache()
{
	clear_highlighting_cache();

	if (text_edit == nullptr) {
		return;
	}
	_update_cache();
}

void CodeHighlighter::set_text_edit(TextEdit* p_text_edit) { text_edit = p_text_edit; }

TextEdit* CodeHighlighter::get_text_edit() const { return text_edit; }

void CodeHighlighter::add_color_region(
	const String& p_start_key, const String& p_end_key, const Color& p_color, bool p_line_only)
{
	for (int i = 0; i < p_start_key.length(); i++) {
		ERR_FAIL_COND_MSG(!is_symbol(p_start_key[i]), "color regions must start with a symbol");
	}

	if (p_end_key.length() > 0) {
		for (int i = 0; i < p_end_key.length(); i++) {
			ERR_FAIL_COND_MSG(!is_symbol(p_end_key[i]), "color regions must end with a symbol");
		}
	}

	int at = 0;
	for (int i = 0; i < color_regions.size(); i++) {
		ERR_FAIL_COND_MSG(color_regions[i].start_key == p_start_key,
			"color region with start key '" + p_start_key + "' already exists.");
		if (p_start_key.length() < color_regions[i].start_key.length()) {
			at++;
		}
	}

	ColorRegion color_region;
	color_region.color = p_color;
	color_region.start_key = p_start_key;
	color_region.end_key = p_end_key;
	color_region.line_only = p_line_only || p_end_key.is_empty();
	color_regions.insert(at, color_region);
	clear_highlighting_cache();
}

void CodeHighlighter::remove_color_region(const String& p_start_key)
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			color_regions.remove_at(i);
			break;
		}
	}
	clear_highlighting_cache();
}

bool CodeHighlighter::has_color_region(const String& p_start_key) const
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			return true;
		}
	}
	return false;
}

void CodeHighlighter::clear_color_regions()
{
	color_regions.clear();
	clear_highlighting_cache();
}

void CodeHighlighter::set_uint_suffix_enabled(bool p_enabled) { uint_suffix_enabled = p_enabled; }

void CodeHighlighter::set_number_color(Color p_color)
{
	number_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_number_color() const { return number_color; }

void CodeHighlighter::set_symbol_color(Color p_color)
{
	symbol_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_symbol_color() const { return symbol_color; }

void CodeHighlighter::set_function_color(Color p_color)
{
	function_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_function_color() const { return function_color; }

void CodeHighlighter::set_member_variable_color(Color p_color)
{
	member_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_member_variable_color() const { return member_color; }

// ============================================================================
// Merged from scene/resources/theme.cpp
// ============================================================================

#include "scene/theme/theme_db.h"
#include "theme.h"

bool Theme::is_valid_type_name(const String& p_name)
{
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

bool Theme::is_valid_item_name(const String& p_name)
{
	if (p_name.is_empty()) {
		return false;
	}
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

String Theme::validate_type_name(const String& p_name)
{
	String type_name = p_name.strip_edges();
	int len = type_name.length();
	char32_t* buffer = type_name.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(buffer[i])) {
			buffer[i] = '_';
		}
	}
	return type_name;
}

// Fallback values for theme item types, configurable per theme.
void Theme::set_default_base_scale(float p_base_scale)
{
	if (default_base_scale == p_base_scale) {
		return;
	}

	default_base_scale = p_base_scale;

	_emit_theme_changed();
}

float Theme::get_default_base_scale() const { return default_base_scale; }

bool Theme::has_default_base_scale() const { return default_base_scale > 0.0; }

Ref<Font> Theme::get_default_font() const { return default_font; }

bool Theme::has_default_font() const { return default_font.is_valid(); }

void Theme::set_default_font_size(int p_font_size)
{
	if (default_font_size == p_font_size) {
		return;
	}

	default_font_size = p_font_size;

	_emit_theme_changed();
}

int Theme::get_default_font_size() const { return default_font_size; }

bool Theme::has_default_font_size() const { return default_font_size > 0; }

Ref<Texture2D> Theme::get_icon(const String& p_name, const String& p_theme_type) const
{
	if (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
		icon_map[p_theme_type][p_name].is_valid()) {
		return icon_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_icon();
	}
}

bool Theme::has_icon(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
			icon_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_icon_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name));
}

void Theme::rename_icon(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!icon_map.has(p_theme_type), "Cannot rename the icon '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(icon_map[p_theme_type].has(p_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!icon_map[p_theme_type].has(p_old_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because it does not exist.");

	icon_map[p_theme_type][p_name] = icon_map[p_theme_type][p_old_name];
	icon_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_icon_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!icon_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Texture2D>>& E : icon_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_icon_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (icon_map.has(p_theme_type)) {
		return;
	}
	icon_map[p_theme_type] = ThemeIconMap();
}

void Theme::rename_icon_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!icon_map.has(p_old_theme_type) || icon_map.has(p_theme_type)) {
		return;
	}

	icon_map[p_theme_type] = icon_map[p_old_theme_type];
	icon_map.erase(p_old_theme_type);
}

void Theme::get_icon_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		p_list->push_back(E.key);
	}
}

Ref<StyleBox> Theme::get_stylebox(const String& p_name, const String& p_theme_type) const
{
	if (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
		style_map[p_theme_type][p_name].is_valid()) {
		return style_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_stylebox();
	}
}

bool Theme::has_stylebox(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
			style_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name));
}

void Theme::rename_stylebox(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!style_map.has(p_theme_type),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(style_map[p_theme_type].has(p_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!style_map[p_theme_type].has(p_old_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because it does not exist.");

	style_map[p_theme_type][p_name] = style_map[p_theme_type][p_old_name];
	style_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_stylebox_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!style_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<StyleBox>>& E : style_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_stylebox_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (style_map.has(p_theme_type)) {
		return;
	}
	style_map[p_theme_type] = ThemeStyleMap();
}

void Theme::rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!style_map.has(p_old_theme_type) || style_map.has(p_theme_type)) {
		return;
	}

	style_map[p_theme_type] = style_map[p_old_theme_type];
	style_map.erase(p_old_theme_type);
}

void Theme::get_stylebox_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		p_list->push_back(E.key);
	}
}

// Fonts.

Ref<Font> Theme::get_font(const String& p_name, const String& p_theme_type) const
{
	if (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
		font_map[p_theme_type][p_name].is_valid()) {
		return font_map[p_theme_type][p_name];
	}
	else if (has_default_font()) {
		return default_font;
	}
	else {
		return ThemeDB::get_fallback_font();
	}
}

bool Theme::has_font(const String& p_name, const String& p_theme_type) const
{
	return ((font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
				font_map[p_theme_type][p_name].is_valid()) ||
			has_default_font());
}

bool Theme::has_font_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
			font_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_font_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name));
}

void Theme::rename_font(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_map.has(p_theme_type), "Cannot rename the font '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_map[p_theme_type].has(p_name),
		"Cannot rename the font '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_map[p_theme_type].has(p_old_name),
		"Cannot rename the font '" + String(p_old_name) + "' because it does not exist.");

	font_map[p_theme_type][p_name] = font_map[p_theme_type][p_old_name];
	font_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_font_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Font>>& E : font_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_map.has(p_theme_type)) {
		return;
	}
	font_map[p_theme_type] = ThemeFontMap();
}

void Theme::rename_font_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_map.has(p_old_theme_type) || font_map.has(p_theme_type)) {
		return;
	}

	font_map[p_theme_type] = font_map[p_old_theme_type];
	font_map.erase(p_old_theme_type);
}

void Theme::get_font_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		p_list->push_back(E.key);
	}
}

// Font sizes.
void Theme::set_font_size(const String& p_name, const String& p_theme_type, int p_font_size)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_font_size_nocheck(p_name, p_theme_type);
	font_size_map[p_theme_type][p_name] = p_font_size;

	_emit_theme_changed(!existing);
}

int Theme::get_font_size(const String& p_name, const String& p_theme_type) const
{
	if (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
		(font_size_map[p_theme_type][p_name] > 0)) {
		return font_size_map[p_theme_type][p_name];
	}
	else if (has_default_font_size()) {
		return default_font_size;
	}
	else {
		return ThemeDB::get_fallback_font_size();
	}
}

bool Theme::has_font_size(const String& p_name, const String& p_theme_type) const
{
	return ((font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
				(font_size_map[p_theme_type][p_name] > 0)) ||
			has_default_font_size());
}

bool Theme::has_font_size_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
			(font_size_map[p_theme_type][p_name] > 0));
}

bool Theme::has_font_size_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name));
}

void Theme::rename_font_size(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot rename the font size '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_size_map[p_theme_type].has(p_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_old_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because it does not exist.");

	font_size_map[p_theme_type][p_name] = font_size_map[p_theme_type][p_old_name];
	font_size_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_font_size(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot clear the font size '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_name),
		"Cannot clear the font size '" + String(p_name) + "' because it does not exist.");

	font_size_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_size_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : font_size_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_size_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_size_map.has(p_theme_type)) {
		return;
	}
	font_size_map[p_theme_type] = ThemeFontSizeMap();
}

void Theme::remove_font_size_type(const String& p_theme_type)
{
	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map.erase(p_theme_type);
}

void Theme::rename_font_size_type(
	const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_size_map.has(p_old_theme_type) || font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map[p_theme_type] = font_size_map[p_old_theme_type];
	font_size_map.erase(p_old_theme_type);
}

void Theme::get_font_size_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		p_list->push_back(E.key);
	}
}

// Colors.
void Theme::set_color(
	const String& p_name, const String& p_theme_type, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_color_nocheck(p_name, p_theme_type);
	color_map[p_theme_type][p_name] = p_color;

	_emit_theme_changed(!existing);
}

Color Theme::get_color(const String& p_name, const String& p_theme_type) const
{
	if (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name)) {
		return color_map[p_theme_type][p_name];
	}
	else {
		return Color();
	}
}

bool Theme::has_color(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

bool Theme::has_color_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

void Theme::rename_color(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type),
		"Cannot rename the color '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(color_map[p_theme_type].has(p_name),
		"Cannot rename the color '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_old_name),
		"Cannot rename the color '" + String(p_old_name) + "' because it does not exist.");

	color_map[p_theme_type][p_name] = color_map[p_theme_type][p_old_name];
	color_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_color(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type), "Cannot clear the color '" + String(p_name) +
														"' because the node type '" +
														String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_name),
		"Cannot clear the color '" + String(p_name) + "' because it does not exist.");

	color_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_color_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!color_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Color>& E : color_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_color_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (color_map.has(p_theme_type)) {
		return;
	}
	color_map[p_theme_type] = ThemeColorMap();
}

void Theme::remove_color_type(const String& p_theme_type)
{
	if (!color_map.has(p_theme_type)) {
		return;
	}

	color_map.erase(p_theme_type);
}

void Theme::rename_color_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!color_map.has(p_old_theme_type) || color_map.has(p_theme_type)) {
		return;
	}

	color_map[p_theme_type] = color_map[p_old_theme_type];
	color_map.erase(p_old_theme_type);
}

void Theme::get_color_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		p_list->push_back(E.key);
	}
}

// Theme constants.
void Theme::set_constant(const String& p_name, const String& p_theme_type, int p_constant)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_constant_nocheck(p_name, p_theme_type);
	constant_map[p_theme_type][p_name] = p_constant;

	_emit_theme_changed(!existing);
}

int Theme::get_constant(const String& p_name, const String& p_theme_type) const
{
	if (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name)) {
		return constant_map[p_theme_type][p_name];
	}
	else {
		return 0;
	}
}

bool Theme::has_constant(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

bool Theme::has_constant_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

void Theme::rename_constant(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot rename the constant '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(constant_map[p_theme_type].has(p_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_old_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because it does not exist.");

	constant_map[p_theme_type][p_name] = constant_map[p_theme_type][p_old_name];
	constant_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_constant(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot clear the constant '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_name),
		"Cannot clear the constant '" + String(p_name) + "' because it does not exist.");

	constant_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_constant_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!constant_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : constant_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_constant_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (constant_map.has(p_theme_type)) {
		return;
	}
	constant_map[p_theme_type] = ThemeConstantMap();
}

void Theme::remove_constant_type(const String& p_theme_type)
{
	if (!constant_map.has(p_theme_type)) {
		return;
	}

	constant_map.erase(p_theme_type);
}

void Theme::rename_constant_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!constant_map.has(p_old_theme_type) || constant_map.has(p_theme_type)) {
		return;
	}

	constant_map[p_theme_type] = constant_map[p_old_theme_type];
	constant_map.erase(p_old_theme_type);
}

void Theme::get_constant_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		p_list->push_back(E.key);
	}
}

bool Theme::has_theme_item(
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		if (!variation_map.has(p_theme_type)) {
			return has_font(p_name, p_theme_type);
		}
		else {
			return has_font_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_FONT_SIZE:
		if (!variation_map.has(p_theme_type)) {
			return has_font_size(p_name, p_theme_type);
		}
		else {
			return has_font_size_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_ICON:
		return has_icon(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

bool Theme::has_theme_item_nocheck(
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color_nocheck(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		return has_font_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return has_font_size_nocheck(p_name, p_theme_type);
	case DATA_TYPE_ICON:
		return has_icon_nocheck(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox_nocheck(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

void Theme::rename_theme_item(DataType p_data_type, const String& p_old_name,
	const String& p_name, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::clear_theme_item(
	DataType p_data_type, const String& p_name, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		clear_color(p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		clear_constant(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		clear_font(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		clear_font_size(p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		clear_icon(p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		clear_stylebox(p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_list(
	DataType p_data_type, const String& p_theme_type, List<String>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::add_theme_item_type(DataType p_data_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		add_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		add_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		add_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		add_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		add_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		add_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::remove_theme_item_type(DataType p_data_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		remove_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		remove_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		remove_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		remove_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		remove_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		remove_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::rename_theme_item_type(
	DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_type_list(p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_type_list(p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_type_list(p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_type_list(p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_type_list(p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_type_list(p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

// Theme type variations.
void Theme::set_type_variation(const String& p_theme_type, const String& p_base_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_base_type), vformat("Invalid type name: '%s'", p_base_type));
	ERR_FAIL_COND_MSG(p_theme_type == String(),
		vformat("An empty theme type cannot be marked as a variation of another type (\"%s\").",
			p_base_type));
	ERR_FAIL_COND_MSG(
		p_base_type == String(), "An empty theme type cannot be the base type of a variation. "
									 "Use clear_type_variation() instead if you want to unmark '" +
										 String(p_theme_type) + "' as a variation.");

	if (variation_map.has(p_theme_type)) {
		String old_base = variation_map[p_theme_type];
		variation_base_map[old_base].erase(p_theme_type);
	}

	variation_map[p_theme_type] = p_base_type;
	variation_base_map[p_base_type].push_back(p_theme_type);

	_emit_theme_changed(true);
}

bool Theme::is_type_variation(const String& p_theme_type, const String& p_base_type) const
{
	return (variation_map.has(p_theme_type) && variation_map[p_theme_type] == p_base_type);
}

void Theme::clear_type_variation(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!variation_map.has(p_theme_type), "Cannot clear the type variation '" +
															String(p_theme_type) +
															"' because it does not exist.");

	String base_type = variation_map[p_theme_type];
	variation_base_map[base_type].erase(p_theme_type);
	variation_map.erase(p_theme_type);

	_emit_theme_changed(true);
}

void Theme::_emit_theme_changed(bool p_notify_list_changed) {}

String Theme::get_type_variation_base(const String& p_theme_type) const
{
	if (!variation_map.has(p_theme_type)) {
		return String();
	}

	return variation_map[p_theme_type];
}

void Theme::get_type_variation_list(const String& p_base_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!variation_base_map.has(p_base_type)) {
		return;
	}

	for (const String& E : variation_base_map[p_base_type]) {
		// Prevent infinite loops if variants were set to be cross-dependent (that's still invalid
		// usage, but handling for stability sake).
		if (p_list->find(E)) {
			continue;
		}

		p_list->push_back(E);
		// Continue looking for sub-variations.
		get_type_variation_list(E, p_list);
	}
}

// Theme types.
void Theme::add_type(const String& p_theme_type)
{
	// Add a record to every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		add_theme_item_type(dt, p_theme_type);
	}

	_emit_theme_changed(true);
}

void Theme::remove_type(const String& p_theme_type)
{
	// Gracefully remove the record from every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		remove_theme_item_type(dt, p_theme_type);
	}

	// If type is a variation, remove that connection.
	if (get_type_variation_base(p_theme_type) != String()) {
		clear_type_variation(p_theme_type);
	}

	// If type is a variation base, remove all those connections.
	List<String> names;
	get_type_variation_list(p_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
	}

	_emit_theme_changed(true);
}

void Theme::rename_type(const String& p_old_theme_type, const String& p_theme_type)
{
	// Gracefully rename the record in every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		rename_theme_item_type(dt, p_old_theme_type, p_theme_type);
	}

	// If type is a variation, replace that connection.
	const String base_type = get_type_variation_base(p_old_theme_type);
	if (base_type != String()) {
		clear_type_variation(p_old_theme_type);
		if (p_theme_type != String()) {
			set_type_variation(p_theme_type, base_type);
		}
	}

	// If type is a variation base, replace all those connections.
	List<String> names;
	get_type_variation_list(p_old_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
		if (p_theme_type != String()) {
			set_type_variation(E, p_theme_type);
		}
	}

	_emit_theme_changed(true);
}

void Theme::get_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	// This Set guarantees uniqueness.
	// Because each map can have the same type defined, but for this method
	// we only want one occurrence of each type.
	HashSet<String> types;

	// Icons.
	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		types.insert(E.key);
	}

	// Styles.
	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		types.insert(E.key);
	}

	// Fonts.
	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		types.insert(E.key);
	}

	// Font sizes.
	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		types.insert(E.key);
	}

	// Colors.
	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		types.insert(E.key);
	}

	// Constants.
	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		types.insert(E.key);
	}

	// Variations.
	for (const KeyValue<String, String>& E : variation_map) {
		types.insert(E.key);
	}

	for (const String& E : types) {
		p_list->push_back(E);
	}
}

void Theme::get_type_dependencies(
	const String& p_base_type, const String& p_type_variation, Vector<String>& r_result)
{
	// Build the dependency chain for type variations.
	if (p_type_variation != String()) {
		String variation_name = p_type_variation;
		while (variation_name != String()) {
			r_result.push_back(variation_name);
			variation_name = get_type_variation_base(variation_name);

			// If we have reached the base type dependency, it's safe to stop (assuming no funny
			// business was done to the Theme).
			if (variation_name == p_base_type) {
				break;
			}
		}
	}

	// Continue building the chain using native class hierarchy.
	ThemeDB::get_native_type_dependencies(p_base_type, r_result);
}

// Internal methods for getting lists as a Vector of String (compatible with public API).
Vector<String> Theme::_get_icon_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_icon_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_color_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_color_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_theme_item_list(DataType p_data_type, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_list(p_theme_type);
	case DATA_TYPE_CONSTANT:
		return _get_constant_list(p_theme_type);
	case DATA_TYPE_FONT:
		return _get_font_list(p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_list(p_theme_type);
	case DATA_TYPE_ICON:
		return _get_icon_list(p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_list(p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_theme_item_type_list(DataType p_data_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_type_list();
	case DATA_TYPE_CONSTANT:
		return _get_constant_type_list();
	case DATA_TYPE_FONT:
		return _get_font_type_list();
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_type_list();
	case DATA_TYPE_ICON:
		return _get_icon_type_list();
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_type_list();
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_type_variation_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_type_variation_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

void Theme::_freeze_change_propagation() { no_change_propagation = true; }

void Theme::_unfreeze_and_propagate_changes()
{
	no_change_propagation = false;
	_emit_theme_changed(true);
}

void Theme::merge_with(const Ref<Theme>& p_other)
{
	if (p_other.is_null()) {
		return;
	}

	_freeze_change_propagation();

	// Colors.
	{
		for (const KeyValue<String, ThemeColorMap>& E : p_other->color_map) {
			for (const KeyValue<String, Color>& F : E.value) {
				set_color(F.key, E.key, F.value);
			}
		}
	}

	// Constants.
	{
		for (const KeyValue<String, ThemeConstantMap>& E : p_other->constant_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_constant(F.key, E.key, F.value);
			}
		}
	}

	// Fonts.
	{
		for (const KeyValue<String, ThemeFontMap>& E : p_other->font_map) {
			for (const KeyValue<String, Ref<Font>>& F : E.value) {
				set_font(F.key, E.key, F.value);
			}
		}
	}

	// Font sizes.
	{
		for (const KeyValue<String, ThemeFontSizeMap>& E : p_other->font_size_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_font_size(F.key, E.key, F.value);
			}
		}
	}

	// Icons.
	{
		for (const KeyValue<String, ThemeIconMap>& E : p_other->icon_map) {
			for (const KeyValue<String, Ref<Texture2D>>& F : E.value) {
				set_icon(F.key, E.key, F.value);
			}
		}
	}

	// Type variations.
	{
		for (const KeyValue<String, String>& E : p_other->variation_map) {
			set_type_variation(E.key, E.value);
		}
	}

	// Defaults.
	if (p_other->has_default_font()) {
		set_default_font(p_other->default_font);
	}
	if (p_other->has_default_font_size()) {
		set_default_font_size(p_other->default_font_size);
	}
	if (p_other->has_default_base_scale()) {
		set_default_base_scale(p_other->default_base_scale);
	}

	_unfreeze_and_propagate_changes();
}

void Theme::reset_state() { clear(); }

Theme::Theme() {}

Theme::~Theme() {}



void Theme::set_icon(String const&, String const&, Ref<Texture2D> const&) {}

void Theme::set_font(String const&, String const&, Ref<Font> const&) {}

void Theme::set_default_font(Ref<Font> const&) {}

void Theme::remove_stylebox_type(String const&) {}

void Theme::remove_icon_type(String const&) {}

void Theme::remove_font_type(String const&) {}

void Theme::clear_stylebox(String const&, String const&) {}

void Theme::clear_icon(String const&, String const&) {}

void Theme::clear_font(String const&, String const&) {}

void Theme::clear() {}

// --- style_box.cpp ---

#include "style_box.h"

#include <cfloat> // FLT_EPSILON
#include "core/config/engine.h"
#include "scene/main/canvas_item.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"

void StyleBox::set_style_type(StyleType p_type)
{
	if (style_type == p_type) {
		return;
	}
	style_type = p_type;
	emit_changed();
}

float StyleBox::get_style_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	switch (style_type) {
	case STYLE_EMPTY:
		return 0.0;
	case STYLE_FLAT:
		return border_width[p_side];
	case STYLE_TEXTURE:
		return texture_margin[p_side];
	case STYLE_LINE:
		if (vertical) {
			if (p_side == SIDE_LEFT || p_side == SIDE_RIGHT) {
				return thickness / 2.0;
			}
		}
		else if (p_side == SIDE_TOP || p_side == SIDE_BOTTOM) {
			return thickness / 2.0;
		}
		return 0.0;
	}
	return 0.0;
}

Size2 StyleBox::get_minimum_size() const
{
	Size2 min_size = Size2(get_margin(SIDE_LEFT) + get_margin(SIDE_RIGHT),
		get_margin(SIDE_TOP) + get_margin(SIDE_BOTTOM));
	Size2 custom_size;
	if (min_size.x < custom_size.x) {
		min_size.x = custom_size.x;
	}
	if (min_size.y < custom_size.y) {
		min_size.y = custom_size.y;
	}

	return min_size;
}

void StyleBox::set_content_margin(Side p_side, float p_value)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	content_margin[p_side] = p_value;
	emit_changed();
}

void StyleBox::set_content_margin_all(float p_value)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = p_value;
	}
	emit_changed();
}

void StyleBox::set_content_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	content_margin[SIDE_LEFT] = p_left;
	content_margin[SIDE_TOP] = p_top;
	content_margin[SIDE_RIGHT] = p_right;
	content_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_content_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return content_margin[p_side];
}

float StyleBox::get_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	if (content_margin[p_side] < 0) {
		return get_style_margin(p_side);
	}
	else {
		return content_margin[p_side];
	}
}

Point2 StyleBox::get_offset() const { return Point2(get_margin(SIDE_LEFT), get_margin(SIDE_TOP)); }

CanvasItem* StyleBox::get_current_item_drawn() const
{
	return CanvasItem::get_current_item_drawn();
}

StyleBox::StyleBox(StyleType p_type) : style_type(p_type)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = -1;
	}
}

Rect2 StyleBox::get_draw_rect(const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_FLAT: {
		Rect2 draw_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);

		if (shadow_size > 0) {
			Rect2 shadow_rect = draw_rect.grow(shadow_size);
			shadow_rect.position += shadow_offset;
			draw_rect = draw_rect.merge(shadow_rect);
		}

		return draw_rect;
	}
	case STYLE_TEXTURE:
		return p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	case STYLE_EMPTY:
	case STYLE_LINE:
	default:
		return p_rect;
	}
}

void StyleBox::draw(RID p_canvas_item, const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_EMPTY:
		return;
	case STYLE_FLAT:
		_draw_flat(p_canvas_item, p_rect);
		return;
	case STYLE_TEXTURE:
		_draw_texture(p_canvas_item, p_rect);
		return;
	case STYLE_LINE:
		_draw_line(p_canvas_item, p_rect);
		return;
	}
}

bool StyleBox::test_mask(const Vector2& p_point, const Rect2& p_rect) const { return true; }

// Shared STYLE_FLAT / STYLE_TEXTURE properties.

void StyleBox::set_expand_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	expand_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_expand_margin_all(float p_expand_margin_size)
{
	for (int i = 0; i < 4; i++) {
		expand_margin[i] = p_expand_margin_size;
	}
	emit_changed();
}

void StyleBox::set_expand_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	expand_margin[SIDE_LEFT] = p_left;
	expand_margin[SIDE_TOP] = p_top;
	expand_margin[SIDE_RIGHT] = p_right;
	expand_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_expand_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);
	return expand_margin[p_side];
}

void StyleBox::set_draw_center(bool p_enabled)
{
	draw_center = p_enabled;
	emit_changed();
}

bool StyleBox::is_draw_center_enabled() const { return draw_center; }

// STYLE_FLAT properties & drawing.

void StyleBox::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	emit_changed();
}

Color StyleBox::get_bg_color() const { return bg_color; }

void StyleBox::set_border_color(const Color& p_color)
{
	border_color = p_color;
	emit_changed();
}

Color StyleBox::get_border_color() const { return border_color; }

void StyleBox::set_border_width_all(int p_size)
{
	border_width[0] = p_size;
	border_width[1] = p_size;
	border_width[2] = p_size;
	border_width[3] = p_size;
	emit_changed();
}

int StyleBox::get_border_width_min() const
{
	return MIN(MIN(border_width[0], border_width[1]), MIN(border_width[2], border_width[3]));
}

void StyleBox::set_border_width(Side p_side, int p_width)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	border_width[p_side] = p_width;
	emit_changed();
}

int StyleBox::get_border_width(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return border_width[p_side];
}

void StyleBox::set_border_blend(bool p_blend)
{
	blend_border = p_blend;
	emit_changed();
}

bool StyleBox::get_border_blend() const { return blend_border; }

void StyleBox::set_corner_radius(const Corner p_corner, const int radius)
{
	ERR_FAIL_INDEX((int)p_corner, 4);
	corner_radius[p_corner] = radius;
	emit_changed();
}

void StyleBox::set_corner_radius_all(int radius)
{
	for (int i = 0; i < 4; i++) {
		corner_radius[i] = radius;
	}

	emit_changed();
}

void StyleBox::set_corner_radius_individual(const int radius_top_left,
	const int radius_top_right, const int radius_bottom_right, const int radius_bottom_left)
{
	corner_radius[0] = radius_top_left;
	corner_radius[1] = radius_top_right;
	corner_radius[2] = radius_bottom_right;
	corner_radius[3] = radius_bottom_left;

	emit_changed();
}

int StyleBox::get_corner_radius(const Corner p_corner) const
{
	ERR_FAIL_INDEX_V((int)p_corner, 4, 0);
	return corner_radius[p_corner];
}

void StyleBox::set_corner_detail(const int& p_corner_detail)
{
	corner_detail = CLAMP(p_corner_detail, 1, 20);
	emit_changed();
}

int StyleBox::get_corner_detail() const { return corner_detail; }

void StyleBox::set_skew(Vector2 p_skew)
{
	skew = p_skew;
	emit_changed();
}

Vector2 StyleBox::get_skew() const { return skew; }

void StyleBox::set_shadow_color(const Color& p_color)
{
	shadow_color = p_color;
	emit_changed();
}

Color StyleBox::get_shadow_color() const { return shadow_color; }

void StyleBox::set_shadow_size(const int& p_size)
{
	shadow_size = p_size;
	emit_changed();
}

int StyleBox::get_shadow_size() const { return shadow_size; }

void StyleBox::set_shadow_offset(const Point2& p_offset)
{
	shadow_offset = p_offset;
	emit_changed();
}

Point2 StyleBox::get_shadow_offset() const { return shadow_offset; }

void StyleBox::set_anti_aliased(const bool& p_anti_aliased)
{
	anti_aliased = p_anti_aliased;
	emit_changed();
}

bool StyleBox::is_anti_aliased() const { return anti_aliased; }

void StyleBox::set_aa_size(const real_t p_aa_size)
{
	aa_size = CLAMP(p_aa_size, 0.01, 10);
	emit_changed();
}

real_t StyleBox::get_aa_size() const { return aa_size; }

static inline void set_inner_corner_radius(const Rect2 style_rect, const Rect2 inner_rect,
	const real_t corner_radius[4], real_t* inner_corner_radius)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	inner_corner_radius[0] = MAX(corner_radius[0] - MIN(border_top, border_left), 0);  // Top left.
	inner_corner_radius[1] = MAX(corner_radius[1] - MIN(border_top, border_right), 0); // Top right.
	inner_corner_radius[2] =
		MAX(corner_radius[2] - MIN(border_bottom, border_right), 0); // Bottom right.
	inner_corner_radius[3] =
		MAX(corner_radius[3] - MIN(border_bottom, border_left), 0); // Bottom left.
}

static inline void set_corner_scale(const Rect2& style_rect, const Rect2& inner_rect,
	const real_t corner_radius[4], Point2* inner_scale)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	// Amount of overflow along an edge.
	// Ex. SIDE_LEFT edge is the overflow between top_left and bottom_left corners.
	// MIN(0,) is to ignore underflow, and negating is to make values positive.
	real_t edge_overflow[4] = {-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_LEFT] -
										   corner_radius[CORNER_BOTTOM_LEFT]),
		-MIN(0,
			inner_rect.size.x - corner_radius[CORNER_TOP_LEFT] - corner_radius[CORNER_TOP_RIGHT]),
		-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_RIGHT] -
					corner_radius[CORNER_BOTTOM_RIGHT]),
		-MIN(0, inner_rect.size.x - corner_radius[CORNER_BOTTOM_LEFT] -
					corner_radius[CORNER_BOTTOM_RIGHT])};

	// Sums of borders.
	real_t hb_sum = border_left + border_right;
	real_t vb_sum = border_top + border_bottom;

	// Ratio of each side to the sum of itself and opposite side.
	// Since overflow only happens with opposite borders, you only need to get the ratio of each
	// border relative to the sum of involved borders.
	real_t ratios[4] = {// Prevent divide by 0 errors.
		hb_sum > 0 ? (border_left / hb_sum) : 0, vb_sum > 0 ? (border_top / vb_sum) : 0,
		hb_sum > 0 ? (border_right / hb_sum) : 0, vb_sum > 0 ? (border_bottom / vb_sum) : 0};

	// Raw amount each corner should shrink.
	Point2 corner_reduction[4] = {
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_BOTTOM]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_BOTTOM]),
	};

	// Corner Radii as Point2s.
	Point2 pcr[4] = {
		Point2(corner_radius[0], corner_radius[0]),
		Point2(corner_radius[1], corner_radius[1]),
		Point2(corner_radius[2], corner_radius[2]),
		Point2(corner_radius[3], corner_radius[3]),
	};

	// If corner radii are too small, they won't shrink the full amount.
	// Adjacent corners will have to shrink the leftovers if they can.
	// Minf(0) is to ignore non-leftovers, and negating is to make values positive.
	Point2 leftovers[4] = {
		-((pcr[0] - corner_reduction[0]).minf(0)),
		-((pcr[1] - corner_reduction[1]).minf(0)),
		-((pcr[2] - corner_reduction[2]).minf(0)),
		-((pcr[3] - corner_reduction[3]).minf(0)),
	};

	// New shrunken radii after distributing the leftovers.
	Point2 distributed[4] = {
		((pcr[0] - corner_reduction[0] - leftovers[3] - leftovers[1]).maxf(0)),
		((pcr[1] - corner_reduction[1] - leftovers[0] - leftovers[2]).maxf(0)),
		((pcr[2] - corner_reduction[2] - leftovers[1] - leftovers[3]).maxf(0)),
		((pcr[3] - corner_reduction[3] - leftovers[2] - leftovers[0]).maxf(0)),
	};

	// How much the curve should scale to achieve the shrunken radii.
	for (int i = 0; i < 4; i++) {
		// Unshrinkable is how much is still left over, even after distributing leftovers.
		// Exclude it from the final scale.
		Point2 unshrinkable =
			(leftovers[(i + 1) % 4] + leftovers[(i + 4 - 1) % 4] - distributed[i]).maxf(0);
		inner_scale[i] = distributed[i] / (pcr[i] - unshrinkable).maxf(FLT_EPSILON);
	}
}

static inline void draw_rounded_rectangle(Vector<Vector2>& verts, Vector<int>& indices,
	Vector<Color>& colors, const Rect2& style_rect, const real_t corner_radius[4],
	const Rect2& ring_rect, const Rect2& inner_rect, const Color& inner_color,
	const Color& outer_color, const int corner_detail, const Vector2& skew, bool is_filled = false)
{
	int vert_offset = verts.size();
	int adapted_corner_detail = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
										(corner_radius[2] > 0) || (corner_radius[3] > 0)
									? corner_detail
									: 1;

	bool draw_border = !is_filled;

	real_t ring_corner_radius[4];
	set_inner_corner_radius(style_rect, ring_rect, corner_radius, ring_corner_radius);

	Point2 ring_scale[4];
	set_corner_scale(style_rect, ring_rect, ring_corner_radius, ring_scale);

	// Corner radius center points.
	Vector<Point2> outer_points = {
		ring_rect.position +
			Vector2(ring_corner_radius[0], ring_corner_radius[0]) * ring_scale[0], // tl
		Point2(ring_rect.position.x + ring_rect.size.x - ring_corner_radius[1] * ring_scale[1].x,
			ring_rect.position.y + ring_corner_radius[1] * ring_scale[1].y), // tr
		ring_rect.position + ring_rect.size -
			Vector2(ring_corner_radius[2], ring_corner_radius[2]) * ring_scale[2], // br
		Point2(ring_rect.position.x + ring_corner_radius[3] * ring_scale[3].x,
			ring_rect.position.y + ring_rect.size.y - ring_corner_radius[3] * ring_scale[3].y) // bl
	};

	real_t inner_corner_radius[4];
	set_inner_corner_radius(style_rect, inner_rect, corner_radius, inner_corner_radius);

	Point2 inner_scale[4];
	set_corner_scale(style_rect, inner_rect, inner_corner_radius, inner_scale);

	Vector<Point2> inner_points = {
		inner_rect.position +
			Vector2(inner_corner_radius[0], inner_corner_radius[0]) * inner_scale[0], // tl
		Point2(
			inner_rect.position.x + inner_rect.size.x - inner_corner_radius[1] * inner_scale[1].x,
			inner_rect.position.y + inner_corner_radius[1] * inner_scale[1].y), // tr
		inner_rect.position + inner_rect.size -
			Vector2(inner_corner_radius[2], inner_corner_radius[2]) * inner_scale[2], // br
		Point2(inner_rect.position.x + inner_corner_radius[3] * inner_scale[3].x,
			inner_rect.position.y + inner_rect.size.y -
				inner_corner_radius[3] * inner_scale[3].y) // bl
	};

	// Calculate the vertices.

	// If the center is filled, we do not draw the border and directly use the inner ring as
	// reference. Because all calls to this method either draw a ring or a filled rounded rectangle,
	// but not both.
	const real_t quarter_arc_rad = Math::PI / 2.0;
	const Point2 style_rect_center = style_rect.get_center();

	const int colors_size = colors.size();
	const int verts_size = verts.size();
	const int new_verts_amount = (adapted_corner_detail + 1) * (draw_border ? 8 : 4);

	colors.resize(colors_size + new_verts_amount);
	verts.resize(verts_size + new_verts_amount);
	Color* colors_ptr = colors.ptrw();
	Vector2* verts_ptr = verts.ptrw();

	for (int corner_idx = 0; corner_idx < 4; corner_idx++) {
		for (int detail = 0; detail <= adapted_corner_detail; detail++) {
			int idx_ofs = (adapted_corner_detail + 1) * corner_idx + detail;
			if (draw_border) {
				idx_ofs *= 2;
			}

			const real_t pt_angle =
				(corner_idx + detail / (double)adapted_corner_detail) * quarter_arc_rad + Math::PI;
			const real_t angle_cosine = Math::cos(pt_angle);
			const real_t angle_sine = Math::sin(pt_angle);

			{
				const real_t x =
					inner_corner_radius[corner_idx] * angle_cosine * inner_scale[corner_idx].x +
					inner_points[corner_idx].x;
				const real_t y =
					inner_corner_radius[corner_idx] * angle_sine * inner_scale[corner_idx].y +
					inner_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs] = inner_color;
			}

			if (draw_border) {
				const real_t x =
					ring_corner_radius[corner_idx] * angle_cosine * ring_scale[corner_idx].x +
					outer_points[corner_idx].x;
				const real_t y =
					ring_corner_radius[corner_idx] * angle_sine * ring_scale[corner_idx].y +
					outer_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs + 1] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs + 1] = outer_color;
			}
		}
	}

	int ring_vert_count = verts.size() - vert_offset;

	// Fill the indices and the colors for the border.

	if (draw_border) {
		int indices_size = indices.size();
		indices.resize(indices_size + ring_vert_count * 3);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < ring_vert_count; i++) {
			int idx_ofs = indices_size + i * 3;
			indices_ptr[idx_ofs] = vert_offset + i % ring_vert_count;
			indices_ptr[idx_ofs + 1] = vert_offset + (i + 2) % ring_vert_count;
			indices_ptr[idx_ofs + 2] = vert_offset + (i + 1) % ring_vert_count;
		}
	}

	if (is_filled) {
		// Compute the triangles pattern to draw the rounded rectangle.
		// Consists of vertical stripes of two triangles each.

		int stripes_count = ring_vert_count / 2 - 1;
		int last_vert_id = ring_vert_count - 1;

		int indices_size = indices.size();
		indices.resize(indices_size + stripes_count * 6);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < stripes_count; i++) {
			int idx_ofs = indices_size + i * 6;
			// Polygon 1.
			indices_ptr[idx_ofs] = vert_offset + i;
			indices_ptr[idx_ofs + 1] = vert_offset + last_vert_id - i - 1;
			indices_ptr[idx_ofs + 2] = vert_offset + i + 1;
			// Polygon 2.
			indices_ptr[idx_ofs + 3] = vert_offset + i;
			indices_ptr[idx_ofs + 4] = vert_offset + last_vert_id - i;
			indices_ptr[idx_ofs + 5] = vert_offset + last_vert_id - i - 1;
		}
	}
}

static inline void adapt_values(int p_index_a, int p_index_b, real_t* adapted_values,
	const real_t* p_values, const real_t p_width, const real_t p_max_a, const real_t p_max_b)
{
	real_t value_a = p_values[p_index_a];
	real_t value_b = p_values[p_index_b];
	real_t factor = MIN(1.0, p_width / (value_a + value_b));
	adapted_values[p_index_a] = MIN(MIN(value_a * factor, p_max_a), adapted_values[p_index_a]);
	adapted_values[p_index_b] = MIN(MIN(value_b * factor, p_max_b), adapted_values[p_index_b]);
}

void StyleBox::_draw_flat(RID p_canvas_item, const Rect2& p_rect) const
{
	bool draw_border = (border_width[0] > 0) || (border_width[1] > 0) || (border_width[2] > 0) ||
					   (border_width[3] > 0);
	bool draw_shadow = (shadow_size > 0);
	if (!draw_border && !draw_center && !draw_shadow) {
		return;
	}

	Rect2 style_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
		expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	if (Math::is_zero_approx(style_rect.size.width) ||
		Math::is_zero_approx(style_rect.size.height)) {
		return;
	}

	const bool rounded_corners = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
								 (corner_radius[2] > 0) || (corner_radius[3] > 0);
	// Only enable antialiasing if it is actually needed. This improves performance
	// and maximizes sharpness for non-skewed StyleBoxes with sharp corners.
	const bool aa_on = (rounded_corners || !skew.is_zero_approx()) && anti_aliased;

	const bool blend_on = blend_border && draw_border;

	Color border_color_alpha = Color(border_color.r, border_color.g, border_color.b, 0);
	Color border_color_blend = (draw_center ? bg_color : border_color_alpha);
	Color border_color_inner = blend_on ? border_color_blend : border_color;

	// Adapt borders (prevent weird overlapping/glitchy drawings).
	real_t width = MAX(style_rect.size.width, 0);
	real_t height = MAX(style_rect.size.height, 0);
	real_t adapted_border[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(SIDE_TOP, SIDE_BOTTOM, adapted_border, border_width, height, height, height);
	adapt_values(SIDE_LEFT, SIDE_RIGHT, adapted_border, border_width, width, width, width);

	// Adapt corners (prevent weird overlapping/glitchy drawings).
	real_t adapted_corner[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(CORNER_TOP_RIGHT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_BOTTOM_LEFT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_TOP_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);
	adapt_values(CORNER_BOTTOM_LEFT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);

	Rect2 infill_rect = style_rect.grow_individual(-adapted_border[SIDE_LEFT],
		-adapted_border[SIDE_TOP], -adapted_border[SIDE_RIGHT], -adapted_border[SIDE_BOTTOM]);

	Rect2 border_style_rect = style_rect;

	real_t aa_size_scaled = 1.0f;
	if (aa_on) {
		real_t scale_factor = TextServer::get_current_drawn_item_oversampling();
		if (scale_factor == 0.0) {
			scale_factor = 1.0;
		}

		// Adjust AA feather size to account for the 2D scale factor, so that
		// antialiasing doesn't become blurry at viewport resolutions higher
		// than the default when using the `canvas_items` stretch mode
		// (or when using `oversampling` values different than `1.0`).
		aa_size_scaled = aa_size / scale_factor;
	}

	if (aa_on) {
		for (int i = 0; i < 4; i++) {
			if (border_width[i] > 0) {
				border_style_rect = border_style_rect.grow_side((Side)i, -aa_size_scaled);
			}
		}
	}

	Vector<Point2> verts;
	Vector<int> indices;
	Vector<Color> colors;
	Vector<Point2> uvs;

	// Create shadow.
	if (draw_shadow) {
		Rect2 shadow_inner_rect = style_rect;
		shadow_inner_rect.position += shadow_offset;

		Rect2 shadow_rect = style_rect.grow(shadow_size);
		shadow_rect.position += shadow_offset;

		Color shadow_color_transparent = Color(shadow_color.r, shadow_color.g, shadow_color.b, 0);

		draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
			shadow_rect, shadow_inner_rect, shadow_color, shadow_color_transparent, corner_detail,
			skew);

		if (draw_center) {
			draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
				shadow_inner_rect, shadow_inner_rect, shadow_color, shadow_color, corner_detail,
				skew, true);
		}
	}

	// Create border (no AA).
	if (draw_border && !aa_on) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			border_style_rect, infill_rect, border_color_inner, border_color, corner_detail, skew);
	}

	// Create infill (no AA).
	if (draw_center && (!aa_on || blend_on)) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			infill_rect, infill_rect, bg_color, bg_color, corner_detail, skew, true);
	}

	if (aa_on) {
		real_t aa_border_width[4];
		real_t aa_border_width_half[4];
		real_t aa_fill_width[4];
		real_t aa_fill_width_half[4];

		if (draw_border) {
			for (int i = 0; i < 4; i++) {
				if (border_width[i] > 0) {
					aa_border_width[i] = aa_size_scaled;
					aa_border_width_half[i] = aa_size_scaled * 0.5;
					aa_fill_width[i] = 0;
					aa_fill_width_half[i] = 0;
				}
				else {
					aa_border_width[i] = 0;
					aa_border_width_half[i] = 0;
					aa_fill_width[i] = aa_size_scaled;
					aa_fill_width_half[i] = aa_size_scaled * 0.5;
				}
			}
		}
		else {
			for (int i = 0; i < 4; i++) {
				aa_border_width[i] = 0;
				aa_border_width_half[i] = 0;
				aa_fill_width[i] = aa_size_scaled;
				aa_fill_width_half[i] = aa_size_scaled * 0.5;
			}
		}

		if (draw_center) {
			// Infill rect, transparent side of antialiasing gradient (base infill rect enlarged by
			// AA size)
			Rect2 infill_rect_aa_transparent = infill_rect.grow_individual(
				aa_fill_width_half[SIDE_LEFT], aa_fill_width_half[SIDE_TOP],
				aa_fill_width_half[SIDE_RIGHT], aa_fill_width_half[SIDE_BOTTOM]);
			// Infill rect, colored side of antialiasing gradient (base infill rect shrunk by AA
			// size)
			Rect2 infill_rect_aa_colored = infill_rect_aa_transparent.grow_individual(
				-aa_fill_width[SIDE_LEFT], -aa_fill_width[SIDE_TOP], -aa_fill_width[SIDE_RIGHT],
				-aa_fill_width[SIDE_BOTTOM]);
			if (!blend_on) {
				// Create center fill, not antialiased yet
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_colored, infill_rect_aa_colored, bg_color, bg_color,
					corner_detail, skew, true);
			}
			if (!blend_on || !draw_border) {
				Color alpha_bg = Color(bg_color.r, bg_color.g, bg_color.b, 0);
				// Add antialiasing on the center fill
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_transparent, infill_rect_aa_colored, bg_color, alpha_bg,
					corner_detail, skew);
			}
		}

		if (draw_border) {
			// Inner border rect, fully colored side of antialiasing gradient (base inner rect
			// enlarged by AA size)
			Rect2 inner_rect_aa_colored = infill_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Inner border rect, transparent side of antialiasing gradient (base inner rect shrunk
			// by AA size)
			Rect2 inner_rect_aa_transparent = inner_rect_aa_colored.grow_individual(
				-aa_border_width[SIDE_LEFT], -aa_border_width[SIDE_TOP],
				-aa_border_width[SIDE_RIGHT], -aa_border_width[SIDE_BOTTOM]);
			// Outer border rect, transparent side of antialiasing gradient (base outer rect
			// enlarged by AA size)
			Rect2 outer_rect_aa_transparent = style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Outer border rect, colored side of antialiasing gradient (base outer rect shrunk by
			// AA size)
			Rect2 outer_rect_aa_colored = border_style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);

			// Create border ring, not antialiased yet
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_colored, ((blend_on) ? infill_rect : inner_rect_aa_colored),
				border_color_inner, border_color, corner_detail, skew);
			if (!blend_on) {
				// Add antialiasing on the ring inner border
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					inner_rect_aa_colored, inner_rect_aa_transparent, border_color_blend,
					border_color, corner_detail, skew);
			}
			// Add antialiasing on the ring outer border
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_transparent, outer_rect_aa_colored, border_color, border_color_alpha,
				corner_detail, skew);
		}
	}

	// Compute UV coordinates.
	Rect2 uv_rect = style_rect.grow(aa_on ? aa_size_scaled : 0);
	uvs.resize(verts.size());
	Point2* uvs_ptr = uvs.ptrw();
	for (int i = 0; i < verts.size(); i++) {
		uvs_ptr[i].x = (verts[i].x - uv_rect.position.x) / uv_rect.size.width;
		uvs_ptr[i].y = (verts[i].y - uv_rect.position.y) / uv_rect.size.height;
	}

	// Draw stylebox.
	Renderer::canvas_item_add_triangle_array(p_canvas_item, indices, verts, colors, uvs);
}

// STYLE_TEXTURE properties & drawing.

void StyleBox::set_texture(Ref<Texture2D> p_texture)
{
	if (texture == p_texture) {
		return;
	}
	texture = p_texture;
	emit_changed();
}

Ref<Texture2D> StyleBox::get_texture() const { return texture; }

void StyleBox::set_texture_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	texture_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_texture_margin_all(float p_size)
{
	for (int i = 0; i < 4; i++) {
		texture_margin[i] = p_size;
	}
	emit_changed();
}

void StyleBox::set_texture_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	texture_margin[SIDE_LEFT] = p_left;
	texture_margin[SIDE_TOP] = p_top;
	texture_margin[SIDE_RIGHT] = p_right;
	texture_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_texture_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return texture_margin[p_side];
}

void StyleBox::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;
	emit_changed();
}

Rect2 StyleBox::get_region_rect() const { return region_rect; }

void StyleBox::set_h_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_h = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_h_axis_stretch_mode() const { return axis_h; }

void StyleBox::set_v_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_v = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_v_axis_stretch_mode() const { return axis_v; }

void StyleBox::set_modulate(const Color& p_modulate)
{
	if (modulate == p_modulate) {
		return;
	}
	modulate = p_modulate;
	emit_changed();
}

Color StyleBox::get_modulate() const { return modulate; }

void StyleBox::_draw_texture(RID p_canvas_item, const Rect2& p_rect) const
{
	if (texture.is_null()) {
		return;
	}

	Rect2 rect = p_rect;
	Rect2 src_rect = region_rect;

	texture->get_rect_region(rect, src_rect, rect, src_rect);

	rect.position.x -= expand_margin[SIDE_LEFT];
	rect.position.y -= expand_margin[SIDE_TOP];
	rect.size.x += expand_margin[SIDE_LEFT] + expand_margin[SIDE_RIGHT];
	rect.size.y += expand_margin[SIDE_TOP] + expand_margin[SIDE_BOTTOM];

	Vector2 start_offset = Vector2(texture_margin[SIDE_LEFT], texture_margin[SIDE_TOP]);
	Vector2 end_offset = Vector2(texture_margin[SIDE_RIGHT], texture_margin[SIDE_BOTTOM]);

	Renderer::canvas_item_add_nine_patch(p_canvas_item, rect, src_rect,
		texture->get_scaled_rid(), start_offset, end_offset, RSE::NinePatchAxisMode(axis_h),
		RSE::NinePatchAxisMode(axis_v), draw_center, modulate);
}

// STYLE_LINE properties & drawing.

void StyleBox::set_color(const Color& p_color)
{
	color = p_color;
	emit_changed();
}

Color StyleBox::get_color() const { return color; }

void StyleBox::set_thickness(int p_thickness)
{
	thickness = p_thickness;
	emit_changed();
}

int StyleBox::get_thickness() const { return thickness; }

void StyleBox::set_vertical(bool p_vertical)
{
	vertical = p_vertical;
	emit_changed();
}

bool StyleBox::is_vertical() const { return vertical; }

void StyleBox::set_grow_end(float p_grow_end)
{
	grow_end = p_grow_end;
	emit_changed();
}

float StyleBox::get_grow_end() const { return grow_end; }

void StyleBox::set_grow_begin(float p_grow_begin)
{
	grow_begin = p_grow_begin;
	emit_changed();
}

float StyleBox::get_grow_begin() const { return grow_begin; }

void StyleBox::_draw_line(RID p_canvas_item, const Rect2& p_rect) const
{
	Rect2i r = p_rect;

	if (vertical) {
		r.position.y -= grow_begin;
		r.size.y += (grow_begin + grow_end);
		r.size.x = thickness;
	}
	else {
		r.position.x -= grow_begin;
		r.size.x += (grow_begin + grow_end);
		r.size.y = thickness;
	}

	Renderer::canvas_item_add_rect(p_canvas_item, r, color);
}
