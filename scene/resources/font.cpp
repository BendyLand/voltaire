/**************************************************************************/
/*  font.cpp                                                              */
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

#include "core/config/engine.h"
#include "core/io/image_loader.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "font.compat.inc"
#include "font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/rendering_server.h"

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
			TS->free_rid(cache[i]);
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

SystemFont::SystemFont()
{ /* NOP */
}

SystemFont::~SystemFont() {}


