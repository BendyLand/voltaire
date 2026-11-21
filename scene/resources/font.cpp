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

#include "label_settings.h"

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
#include "text_file.h"

bool TextFile::has_text() const { return !text.is_empty(); }

String TextFile::get_text() const { return text; }

void TextFile::set_text(const String& p_code) { text = p_code; }

void TextFile::reload_from_file() { load_text(path); }



Error TextFile::load_text(String const&) {}

// --- syntax_highlighter.cpp ---

#include "scene/gui/text_edit.h"
#include "syntax_highlighter.h"

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
