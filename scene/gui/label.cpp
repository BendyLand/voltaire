/**************************************************************************/
/*  label.cpp                                                             */
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

#include "label.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"

TextServer::AutowrapMode Label::get_autowrap_mode() const { return autowrap_mode; }

uint32_t Label::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

uint32_t Label::get_justification_flags() const { return jst_flags; }

bool Label::is_uppercase() const { return uppercase; }

int Label::get_line_height(int p_line) const
{
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	if (p_line >= 0 && p_line < total_line_count) {
		RID rid = get_line_rid(p_line);
		double asc = TS->shaped_text_get_ascent(rid);
		double dsc = TS->shaped_text_get_descent(rid);
		if (asc + dsc < font_h) {
			double diff = font_h - (asc + dsc);
			asc += diff / 2;
			dsc += diff - (diff / 2);
		}
		return asc + dsc;
	}
	else if (total_line_count > 0) {
		int h = font_h;
		for (const Paragraph& para : paragraphs) {
			for (const RID& line_rid : para.lines_rid) {
				h = MAX(h, TS->shaped_text_get_size(line_rid).y);
			}
		}
		return h;
	}
	else {
		return font->get_height(font_size);
	}
}

void Label::_update_visible() const
{
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int lines_visible = total_line_count;

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	minsize.height = 0;
	int last_line = MIN(total_line_count, lines_visible + lines_skipped);

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size()
																	   : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				minsize.height += asc + dsc + line_spacing;
			}
			minsize.height += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (minsize.height > 0) {
		minsize.height -= (line_spacing + paragraph_spacing);
	}
}

inline void draw_glyph(
	const Glyph& p_gl, const RID& p_canvas, const Color& p_font_color, const Vector2& p_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_color);
	}
	else if (((p_gl.flags & TextServer::GRAPHEME_IS_VIRTUAL) !=
				   TextServer::GRAPHEME_IS_VIRTUAL) &&
			   ((p_gl.flags & TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) !=
				   TextServer::GRAPHEME_IS_EMBEDDED_OBJECT)) {
		TS->draw_hex_code_box(p_canvas, p_gl.font_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off),
			p_gl.index, p_font_color);
	}
}

inline void draw_glyph_shadow(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_shadow_color, const Vector2& p_ofs, const Vector2& shadow_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + shadow_ofs, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_shadow_outline(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_shadow_color, const Vector2& p_ofs, int p_shadow_outline_size,
	const Vector2& shadow_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_shadow_outline_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + shadow_ofs, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_outline(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_outline_color, const Vector2& p_ofs, int p_outline_size)
{
	if (p_gl.font_rid != RID()) {
		if (p_font_outline_color.a != 0.0 && p_outline_size > 0) {
			TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_outline_size,
				p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_outline_color);
		}
	}
}

void Label::_ensure_shaped() const
{
	if (dirty || font_dirty || text_dirty) {
		_shape();
	}
	else {
		for (const Paragraph& para : paragraphs) {
			if (para.lines_dirty || para.dirty) {
				_shape();
				return;
			}
		}
	}
}

RID Label::get_line_rid(int p_line) const
{
	if (p_line < 0 || p_line >= total_line_count) {
		return RID();
	}

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		}
		else {
			return para.lines_rid[p_line - line_index];
		}
	}

	return RID();
}

Rect2 Label::get_line_rect(int p_line) const
{
	if (p_line < 0 || p_line >= total_line_count) {
		return Rect2();
	}

	int line_index = 0;
	for (int p = 0; p < paragraphs.size(); p++) {
		const Paragraph& para = paragraphs[p];
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		}
		else {
			return _get_line_rect(p, p_line - line_index);
		}
	}

	return Rect2();
}

Rect2 Label::_get_line_rect(int p_para, int p_line) const
{
	// Returns a rect providing the line's horizontal offset and total size. To determine the
	// vertical offset, use r_offset and r_line_spacing from get_layout_data.
	bool rtl = TS->shaped_text_get_inferred_direction(paragraphs[p_para].text_rid) ==
			   TextServer::DIRECTION_RTL;
	bool rtl_layout = is_layout_rtl();
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	Size2 size = get_size();
	RID rid = paragraphs[p_para].lines_rid[p_line];
	Size2 line_size = TS->shaped_text_get_size(rid);
	double asc = TS->shaped_text_get_ascent(rid);
	double dsc = TS->shaped_text_get_descent(rid);
	if (asc + dsc < font_h) {
		double diff = font_h - (asc + dsc);
		asc += diff / 2;
		dsc += diff - (diff / 2);
	}
	line_size.y = asc + dsc;
	Vector2 offset;
	switch (horizontal_alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
		if (rtl && autowrap_mode != TextServer::AUTOWRAP_OFF) {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
		else {
			offset.x = style->get_offset().x;
		}
		break;
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl_layout) {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
		else {
			offset.x = style->get_offset().x;
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		offset.x = int(size.width - line_size.width) / 2;
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl_layout) {
			offset.x = style->get_offset().x;
		}
		else {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
	} break;
	}
	return Rect2(offset, line_size);
}

int Label::get_layout_data(Vector2& r_offset, int& r_last_line, int& r_line_spacing) const
{
	// Computes several common parameters involved in laying out and rendering text set to this
	// label. Only vertical margin is considered in r_offset: use get_line_rect to get the
	// horizontal offset for a given line of text.
	Size2 size = get_size();
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	float total_h = 0.0;
	int lines_visible = 0;

	// Get number of lines to fit to the height.
	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines_rid.size(); i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				if (total_h > Math::ceil(get_size().height - style->get_minimum_size().height +
										 line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	r_last_line = MIN(total_line_count, lines_visible + lines_skipped);

	// Get real total height.
	int total_glyphs = 0;
	total_h = 0;
	line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < r_last_line) ? para.lines_rid.size()
																		 : r_last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				total_glyphs += TS->shaped_text_get_glyph_count(para.lines_rid[i]) +
								TS->shaped_text_get_ellipsis_glyph_count(para.lines_rid[i]);
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	total_h += style->get_margin(SIDE_TOP) + style->get_margin(SIDE_BOTTOM);
	int vbegin = 0, vsep = 0;
	if (lines_visible > 0) {
		switch (vertical_alignment) {
		case VERTICAL_ALIGNMENT_TOP: {
			// Nothing.
		} break;
		case VERTICAL_ALIGNMENT_CENTER: {
			vbegin = (size.y - (total_h - line_spacing - paragraph_spacing)) / 2;
			vsep = 0;
		} break;
		case VERTICAL_ALIGNMENT_BOTTOM: {
			vbegin = size.y - (total_h - line_spacing - paragraph_spacing);
			vsep = 0;
		} break;
		case VERTICAL_ALIGNMENT_FILL: {
			vbegin = 0;
			if (lines_visible > 1) {
				vsep =
					(size.y - (total_h - line_spacing - paragraph_spacing)) / (lines_visible - 1);
			}
			else {
				vsep = 0;
			}
		} break;
		}
	}
	r_offset = {0, style->get_offset().y + vbegin};
	r_line_spacing = line_spacing + vsep;

	return total_glyphs;
}

PackedStringArray Label::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	// FIXME: This is not ideal and the sizing model should be fixed,
	// but for now we have to warn about this impossible to resolve combination.
	// See GH-83546.
	if (is_inside_tree() && get_tree()->get_edited_scene_root() != this) {
		if (autowrap_mode != TextServer::AUTOWRAP_OFF && get_combined_maximum_size().width <= 0 &&
			get_custom_minimum_size().width <= 0) {
			warnings.push_back(RTR("Labels with autowrapping enabled must have a positive custom "
								   "minimum or maximum width configured to work correctly."));
		}
	}

	// Ensure that the font can render all of the required glyphs.
	Ref<Font> font;
	if (settings.is_valid()) {
		font = settings->get_font();
	}
	if (font.is_null()) {
		font = theme_cache.font;
	}

	if (font.is_valid()) {
		_ensure_shaped();

		for (const Paragraph& para : paragraphs) {
			const Glyph* glyph = TS->shaped_text_get_glyphs(para.text_rid);
			int64_t glyph_count = TS->shaped_text_get_glyph_count(para.text_rid);
			for (int64_t i = 0; i < glyph_count; i++) {
				if (glyph[i].font_rid == RID() && glyph[i].index != 0) {
					warnings.push_back(RTR("The current font does not support rendering one or "
										   "more characters used in this Label's text."));
					break;
				}
			}
		}
	}

	Ref<FontFile> ff = font;
	if (ff.is_valid() && ff->is_multichannel_signed_distance_field()) {
		bool has_settings = settings.is_valid();
		int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
		int outline_size =
			has_settings ? settings->get_outline_size() : theme_cache.font_outline_size;
		Vector<LabelSettings::StackedOutlineData> stacked_outline_datas =
			has_settings ? settings->get_stacked_outline_data()
						 : Vector<LabelSettings::StackedOutlineData>();
		Vector<LabelSettings::StackedShadowData> stacked_shadow_datas =
			has_settings ? settings->get_stacked_shadow_data()
						 : Vector<LabelSettings::StackedShadowData>();
		int max_outline_draw_size = outline_size;
		if (stacked_outline_datas.size() != 0) {
			int draw_iterations = stacked_outline_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				int stacked_outline_size = stacked_outline_datas[j].size;
				if (stacked_outline_size <= 0) {
					continue;
				}
				max_outline_draw_size += stacked_outline_size;
			}
		}
		if (stacked_shadow_datas.size() != 0) {
			int draw_iterations = stacked_shadow_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				LabelSettings::StackedShadowData stacked_shadow_data = stacked_shadow_datas[j];
				if (stacked_shadow_data.outline_size > 0) {
					max_outline_draw_size =
						MAX(max_outline_draw_size, stacked_shadow_data.outline_size);
				}
			}
		}
		float scale = (float)font_size / (float)ff->get_msdf_size();
		float ol = (float)max_outline_draw_size / scale / 4.0;
		float pxr = (float)ff->get_msdf_pixel_range() / 2.0 - 1.0;
		float r_pxr = (ol + 1.0) * 2.0;

		if (ol > pxr) {
			warnings.push_back(vformat(
				RTR("MSDF font pixel range is too small, some outlines/shadows will not render. "
					"Set MSDF pixel range to be at least %d to render all outlines/shadows."),
				Math::ceil(r_pxr)));
		}
	}

	return warnings;
}

Rect2 Label::get_character_bounds(int p_pos) const
{
	_ensure_shaped();

	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);

	Vector2 ofs;
	int line_spacing;
	int last_line;
	get_layout_data(ofs, last_line, line_spacing);

	int line_index = 0;
	for (int p = 0; p < paragraphs.size(); p++) {
		const Paragraph& para = paragraphs[p];
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size()
																	   : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				RID line_rid = para.lines_rid[i];
				Rect2 line_rect = _get_line_rect(p, i);
				ofs.x = line_rect.position.x;

				int v_size = TS->shaped_text_get_glyph_count(line_rid);
				const Glyph* glyphs = TS->shaped_text_get_glyphs(line_rid);

				float gl_off = 0.0f;
				for (int j = 0; j < v_size; j++) {
					if ((glyphs[j].count > 0) &&
						((glyphs[j].index != 0) ||
							((glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) ==
								TextServer::GRAPHEME_IS_SPACE))) {
						if (p_pos >= glyphs[j].start + para.start &&
							p_pos < glyphs[j].end + para.start) {
							float advance = 0.f;
							for (int k = 0; k < glyphs[j].count; k++) {
								advance += glyphs[j + k].advance;
							}
							Rect2 rect;
							rect.position = ofs + Vector2(gl_off, 0);
							rect.size = Vector2(advance, line_rect.size.y);
							return rect;
						}
					}
					gl_off += glyphs[j].advance * glyphs[j].repeat;
				}
				double asc = TS->shaped_text_get_ascent(line_rid);
				double dsc = TS->shaped_text_get_descent(line_rid);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				ofs.y += asc + dsc + line_spacing;
			}
			ofs.y += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	return Rect2();
}

Size2 Label::get_minimum_size() const
{
	_ensure_shaped();

	Size2 min_size = minsize;

	const Ref<Font>& font = (settings.is_valid() && settings->get_font().is_valid())
								? settings->get_font()
								: theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;

	min_size.height = MAX(min_size.height, font->get_height(font_size));

	Size2 min_style = theme_cache.normal_style->get_minimum_size();
	if (autowrap_mode != TextServer::AUTOWRAP_OFF) {
		if (!clip && overrun_behavior != TextServer::OVERRUN_NO_TRIMMING && max_lines_visible > 0) {
			int line_spacing =
				settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
			min_size.height = MIN(
				min_size.height, (font->get_height(font_size) + line_spacing) * max_lines_visible);

		}
		else if (clip || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			min_size.height = 1;
		}
		return Size2(1, min_size.height) + min_style;
	}
	else {
		if (clip || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			min_size.width = 1;
		}
		return min_size + min_style;
	}
}

Size2 Label::get_desired_size() const
{
	Size2 combined_max = get_combined_maximum_size();
	if (combined_max.width < 0) {
		return Size2();
	}

	_ensure_shaped();
	Size2 min_style = theme_cache.normal_style->get_minimum_size();
	Size2 content_size = minsize + min_style;
	content_size.width = MIN(content_size.width, int(combined_max.width));

	return content_size;
}

int Label::get_line_count() const
{
	if (!is_inside_tree()) {
		return 1;
	}
	_ensure_shaped();

	return total_line_count;
}

int Label::get_visible_line_count() const
{
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	int lines_visible = 0;
	float total_h = 0.0;

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines_rid.size(); i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				if (total_h > Math::ceil(get_size().height - style->get_minimum_size().height +
										 line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	return lines_visible;
}

HorizontalAlignment Label::get_horizontal_alignment() const { return horizontal_alignment; }

VerticalAlignment Label::get_vertical_alignment() const { return vertical_alignment; }

Ref<LabelSettings> Label::get_label_settings() const { return settings; }

TextServer::StructuredTextParser Label::get_structured_text_bidi_override() const
{
	return st_parser;
}

Control::TextDirection Label::get_text_direction() const { return text_direction; }

String Label::get_language() const { return language; }

String Label::get_paragraph_separator() const { return paragraph_separator; }

bool Label::is_clipping_text() const { return clip; }

PackedFloat32Array Label::get_tab_stops() const { return tab_stops; }

TextServer::OverrunBehavior Label::get_text_overrun_behavior() const { return overrun_behavior; }

String Label::get_ellipsis_char() const { return el_char; }

String Label::get_text() const { return text; }

int Label::get_visible_characters() const { return visible_chars; }

float Label::get_visible_ratio() const { return visible_ratio; }

TextServer::VisibleCharactersBehavior Label::get_visible_characters_behavior() const
{
	return visible_chars_behavior;
}

int Label::get_lines_skipped() const { return lines_skipped; }

int Label::get_max_lines_visible() const { return max_lines_visible; }

int Label::get_total_character_count() const { return xl_text.length(); }

Label::~Label()
{
	for (Paragraph& para : paragraphs) {
		for (const RID& line_rid : para.lines_rid) {
			TS->free_rid(line_rid);
		}
		para.lines_rid.clear();
		TS->free_rid(para.text_rid);
	}
	paragraphs.clear();
}


