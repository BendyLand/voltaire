/**************************************************************************/
/*  label.cpp                                                             */
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

#include "label.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/alt_codes.h"
#include "line_edit.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "item_list.h"
#include "scene/resources/atlas_texture.h"
#include "texture_rect.h"
#include "nine_patch_rect.h"
#include "reference_rect.h"
#include "servers/audio/audio_server.h"
#include "video_stream_player.h"
#include "virtual_joystick.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

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
	if (is_inside_tree() && SceneTree::get_edited_scene_root() != this) {
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

void Label::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior) {}

TextServer::OverrunBehavior Label::get_text_overrun_behavior() const { return overrun_behavior; }

String Label::get_ellipsis_char() const { return el_char; }

String Label::get_text() const { return text; }

void Label::set_text(const String& p_string) { text = p_string; }

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

void Label::set_horizontal_alignment(HorizontalAlignment p_alignment) {}

void Label::set_autowrap_mode(TextServer::AutowrapMode) {}

void Label::_shape() const {}

void Label::set_vertical_alignment(VerticalAlignment) {}

// --- line_edit.cpp ---

void LineEdit::edit(bool p_hide_focus) { _edit(true, p_hide_focus); }

void LineEdit::unedit()
{
	if (!editing) {
		return;
	}

	editing = false;
	_validate_caret_can_draw();

	apply_ime();
	set_caret_column(caret_column); // Update scroll_offset.

	if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		DisplayServer::virtual_keyboard_hide();
	}

	if (deselect_on_focus_loss_enabled && !selection.drag_attempt) {
		deselect();
	}
}

bool LineEdit::is_editing() const { return editing; }

void LineEdit::set_keep_editing_on_text_submit(bool p_enabled)
{
	keep_editing_on_text_submit = p_enabled;
}

bool LineEdit::is_editing_kept_on_text_submit() const { return keep_editing_on_text_submit; }

void LineEdit::_close_ime_window()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::window_set_ime_position(Point2(), wid);
	DisplayServer::window_set_ime_active(false, wid);
}

void LineEdit::_update_ime_window_position()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::window_set_ime_active(true, wid);
	Point2 pos = Point2(get_caret_pixel_pos().x,
					 (get_size().y + theme_cache.font->get_height(theme_cache.font_size)) / 2) +
				 get_global_position();
	if (get_window()->get_embedder()) {
		pos += get_viewport()->get_popup_base_transform().get_origin();
	}
	// Take into account the window's transform.
	pos = get_window()->get_screen_transform().xform(pos);
	// The window will move to the updated position the next time the IME is updated, not
	// immediately.
	DisplayServer::window_set_ime_position(pos, wid);
}

bool LineEdit::has_ime_text() const { return !ime_text.is_empty(); }

void LineEdit::cancel_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}
	ime_text = String();
	ime_selection = Vector2i();
	alt_start = false;
	alt_start_no_hold = false;
	_close_ime_window();
	_shape();
}

void LineEdit::apply_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}

	// Force apply the current IME text.
	if (alt_start || alt_start_no_hold) {
		cancel_ime();
		if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff && alt_code <= 0x10ffff)) {
			char32_t ucodestr[2] = {(char32_t)alt_code, 0};
			insert_text_at_caret(ucodestr);
		}
	}
	else {
		String insert_ime_text = ime_text;
		cancel_ime();
		insert_text_at_caret(insert_ime_text);
	}
}

void LineEdit::_swap_current_input_direction()
{
	if (input_direction == TEXT_DIRECTION_LTR) {
		input_direction = TEXT_DIRECTION_RTL;
	}
	else {
		input_direction = TEXT_DIRECTION_LTR;
	}
	set_caret_column(get_caret_column());
}

void LineEdit::_move_caret_left(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.begin);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Move to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() - 1);
		}
		else {
			set_caret_column(TS->shaped_text_prev_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_right(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.end);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Move to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() + 1);
		}
		else {
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_start(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(0);
	shift_selection_check_post(p_select);
}

void LineEdit::_move_caret_end(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(text.length());
	shift_selection_check_post(p_select);
}

void LineEdit::_backspace(bool p_word, bool p_all_to_left)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == 0) {
		return; // Nothing to do.
	}

	if (p_all_to_left) {
		text = text.substr(caret_column);
		_shape();
		set_caret_column(0);
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Delete to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(cc, caret_column);

		set_caret_column(cc);
	}
	else {
		delete_char();
	}
}

void LineEdit::_delete(bool p_word, bool p_all_to_right)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == text.length()) {
		return; // Nothing to do.
	}

	if (p_all_to_right) {
		text = text.substr(0, caret_column);
		_shape();
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;
		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Delete to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(caret_column, cc);
		set_caret_column(caret_column);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(caret_column + 1);
			delete_char();
		}
		else {
			int cc = caret_column;
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, caret_column));
			delete_text(cc, caret_column);
		}
	}
}

Point2 LineEdit::_get_right_icon_size(Ref<Texture2D> p_right_icon) const
{
	Size2 icon_size;

	if (p_right_icon.is_null()) {
		return icon_size;
	}

	switch (icon_expand_mode) {
	default:
	case LineEdit::EXPAND_MODE_ORIGINAL_SIZE:
		icon_size = p_right_icon->get_size();
		break;
	case LineEdit::EXPAND_MODE_FIT_TO_TEXT: {
		real_t height = theme_cache.font->get_height(theme_cache.font_size);
		icon_size = Size2(height, height);
	} break;
	case LineEdit::EXPAND_MODE_FIT_TO_LINE_EDIT: {
		icon_size = p_right_icon->get_size();
		Point2 size = get_size();
		float icon_width = icon_size.width * size.height / icon_size.height;
		float icon_height = size.height;

		if (icon_width > size.width) {
			icon_width = size.width;
			icon_height = icon_size.height * icon_width / icon_size.width;
		}

		icon_size = Size2(icon_width, icon_height) * right_icon_scale;
	} break;
	}

	return icon_size;
}

void LineEdit::unhandled_key_input(const InputEvent& p_event)
{
	// Return to prevent editing if just focused.
	if (!editing) {
		return;
	}

	if (p_event.type == InputEventType::KEY) {
		if (!p_event.is_pressed()) {
			return;
		}
		// Handle Unicode (with modifiers active, process after shortcuts).
		if (has_focus() && editable && (p_event.key.unicode >= 32)) {
			selection_delete();
			char32_t ucodestr[2] = {p_event.key.unicode, 0};
			int prev_len = text.length();
			insert_text_at_caret(ucodestr);
			if (text.length() != prev_len) {
				_text_changed();
			}
			accept_event();
		}
	}
}

HorizontalAlignment LineEdit::get_horizontal_alignment() const { return alignment; }

Control::CursorShape LineEdit::get_cursor_shape(const Point2& p_pos) const
{
	if ((!text.is_empty() && is_editable() && _is_over_clear_button(p_pos)) ||
		(!is_editable() && (!is_selecting_enabled() || text.is_empty()))) {
		return CURSOR_ARROW;
	}
	return Control::get_cursor_shape(p_pos);
}

bool LineEdit::_is_over_clear_button(const Point2& p_pos) const
{
	if (!clear_button_enabled || !has_point(p_pos)) {
		return false;
	}
	Ref<Texture2D> icon = theme_cache.clear_icon;
	return is_layout_rtl() ? p_pos.x < theme_cache.normal->get_margin(SIDE_LEFT) + icon->get_width()
						   : p_pos.x > get_size().width - icon->get_width() -
										   theme_cache.normal->get_margin(SIDE_RIGHT);
}

void LineEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void LineEdit::copy_text()
{
	if (selection.enabled && !pass) {
		DisplayServer::clipboard_set(get_selected_text());
	}
}

void LineEdit::cut_text()
{
	if (editable && selection.enabled && !pass) {
		DisplayServer::clipboard_set(get_selected_text());
		selection_delete();
	}
}

bool LineEdit::has_undo() const
{
	if (undo_stack_pos == nullptr) {
		return undo_stack.size() > 1;
	}
	return undo_stack_pos != undo_stack.front();
}

bool LineEdit::has_redo() const
{
	return undo_stack_pos != nullptr && undo_stack_pos != undo_stack.back();
}

void LineEdit::deselect() {}

void LineEdit::undo()
{
	if (!editable) {
		return;
	}

	if (!has_undo()) {
		return;
	}

	if (undo_stack_pos == nullptr) {
		undo_stack_pos = undo_stack.back();
	}

	deselect();

	undo_stack_pos = undo_stack_pos->prev();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::redo()
{
	if (!editable) {
		return;
	}

	if (!has_redo()) {
		return;
	}

	deselect();

	undo_stack_pos = undo_stack_pos->next();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::shift_selection_check_pre(bool p_shift)
{
	if (!selection.enabled && p_shift) {
		selection.start_column = caret_column;
	}
	if (!p_shift) {
		deselect();
	}
}

void LineEdit::shift_selection_check_post(bool p_shift)
{
	if (p_shift) {
		selection_fill_at_caret();
	}
}

void LineEdit::set_caret_at_pixel_pos(int p_x)
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	int ofs = std::ceil(TS->shaped_text_hit_test_position(text_rid, p_x - x_ofs - scroll_offset));
	if (ofs == -1) {
		return;
	}
	if (!caret_mid_grapheme_enabled) {
		ofs = TS->shaped_text_closest_character_pos(text_rid, ofs);
	}
	set_caret_column(ofs);
}

Vector2 LineEdit::get_caret_pixel_pos()
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	Vector2 ret;
	CaretInfo caret;
	// Get position of the start of caret.
	if (!ime_text.is_empty() && ime_selection.x != 0) {
		caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_selection.x);
	}
	else {
		caret = TS->shaped_text_get_carets(text_rid, caret_column);
	}

	if ((caret.l_caret != Rect2() && (caret.l_dir == TextServer::DIRECTION_AUTO ||
										 caret.l_dir == (TextServer::Direction)input_direction)) ||
		(caret.t_caret == Rect2())) {
		ret.x = x_ofs + caret.l_caret.position.x + scroll_offset;
	}
	else {
		ret.x = x_ofs + caret.t_caret.position.x + scroll_offset;
	}

	// Get position of the end of caret.
	if (!ime_text.is_empty()) {
		if (ime_selection.y != 0) {
			caret = TS->shaped_text_get_carets(
				text_rid, caret_column + ime_selection.x + ime_selection.y);
		}
		else {
			caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_text.size());
		}
		if ((caret.l_caret != Rect2() &&
				(caret.l_dir == TextServer::DIRECTION_AUTO ||
					caret.l_dir == (TextServer::Direction)input_direction)) ||
			(caret.t_caret == Rect2())) {
			ret.y = x_ofs + caret.l_caret.position.x + scroll_offset;
		}
		else {
			ret.y = x_ofs + caret.t_caret.position.x + scroll_offset;
		}
	}
	else {
		ret.y = ret.x;
	}

	return ret;
}

void LineEdit::set_caret_mid_grapheme_enabled(const bool p_enabled)
{
	caret_mid_grapheme_enabled = p_enabled;
}

bool LineEdit::is_caret_mid_grapheme_enabled() const { return caret_mid_grapheme_enabled; }

bool LineEdit::is_caret_blink_enabled() const { return caret_blink_enabled; }

bool LineEdit::is_caret_force_displayed() const { return caret_force_displayed; }

float LineEdit::get_caret_blink_interval() const { return caret_blink_interval; }

void LineEdit::set_caret_blink_interval(const float p_interval)
{
	ERR_FAIL_COND(p_interval <= 0);
	caret_blink_interval = p_interval;
}

void LineEdit::_validate_caret_can_draw()
{
	if (caret_blink_enabled) {
		draw_caret = true;
		caret_blink_timer = 0.0;
	}
	caret_can_draw = (caret_force_displayed && !is_part_of_edited_scene()) ||
					 (editing && (window_has_focus || (menu && menu->has_focus())) && has_focus());
}

void LineEdit::delete_char()
{
	if (text.is_empty() || caret_column == 0) {
		return;
	}
	int delete_char_offset = 1;
	if (!caret_mid_grapheme_enabled && backspace_deletes_composite_character_enabled) {
		delete_char_offset = caret_column - get_previous_composite_character_column(caret_column);
	}
	text = text.left(caret_column - delete_char_offset) + text.substr(caret_column);
	_shape();

	set_caret_column(get_caret_column() - delete_char_offset);

	_text_changed();
}

void LineEdit::set_text(String p_text) { _set_text(p_text); }

Control::TextDirection LineEdit::get_text_direction() const { return text_direction; }

String LineEdit::get_language() const { return language; }

bool LineEdit::get_draw_control_chars() const { return draw_control_chars; }

TextServer::StructuredTextParser LineEdit::get_structured_text_bidi_override() const
{
	return st_parser;
}

void LineEdit::clear()
{
	bool was_empty = text.is_empty();
	clear_internal();
	_clear_redo();
	if (!was_empty) {
		_emit_text_change();
	}

	// This should reset virtual keyboard state if needed.
	if (editing) {
		show_virtual_keyboard();
	}
}

void LineEdit::show_virtual_keyboard()
{
	_update_ime_window_position();

	if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		if (selection.enabled) {
			DisplayServer::virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				selection.begin, selection.end);
		}
		else {
			DisplayServer::virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				caret_column);
		}
	}
}

String LineEdit::get_text() const { return text; }

String LineEdit::get_placeholder() const { return placeholder; }

int LineEdit::get_caret_column() const { return caret_column; }

int LineEdit::get_next_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == text.length()) {
		return p_column;
	}
	else {
		return TS->shaped_text_next_character_pos(text_rid, p_column);
	}
}

int LineEdit::get_previous_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == 0) {
		return 0;
	}
	else {
		return TS->shaped_text_prev_character_pos(text_rid, p_column);
	}
}

void LineEdit::set_scroll_offset(float p_pos)
{
	scroll_offset = p_pos;
	if (scroll_offset < 0.0) {
		scroll_offset = 0.0;
	}
}

float LineEdit::get_scroll_offset() const { return scroll_offset; }

Size2 LineEdit::get_minimum_size() const
{
	Ref<Font> font = theme_cache.font;
	int font_size = theme_cache.font_size;

	Size2 min_size;

	// Minimum size of text.
	// W is wider than M in most fonts, Using M may result in hiding the last digit when using float
	// values in SpinBox, ie. ColorPicker RAW values.
	float em_space_size = font->get_char_size('W', font_size).x;
	min_size.width = theme_cache.minimum_character_width * em_space_size;

	if (expand_to_text_length) {
		// Ensure some space for the caret when placed at the end.
		min_size.width = MAX(min_size.width, full_width + theme_cache.caret_width);
	}

	min_size.height = MAX(TS->shaped_text_get_size(text_rid).y, font->get_height(font_size));

	// Take icons into account.
	int icon_max_width = 0;
	if (right_icon.is_valid()) {
		Point2 right_icon_size = _get_right_icon_size(right_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = right_icon_size.width;
	}
	if (clear_button_enabled) {
		Point2 right_icon_size = _get_right_icon_size(theme_cache.clear_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = MAX(icon_max_width, right_icon_size.width);
	}
	min_size.width += icon_max_width;

	Size2 style_min_size =
		theme_cache.normal->get_minimum_size().max(theme_cache.read_only->get_minimum_size());
	return style_min_size + min_size;
}

bool LineEdit::has_selection() const { return selection.enabled; }

String LineEdit::get_selected_text()
{
	if (selection.enabled) {
		return text.substr(selection.begin, selection.end - selection.begin);
	}
	else {
		return String();
	}
}

int LineEdit::get_selection_from_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.begin;
}

int LineEdit::get_selection_to_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.end;
}

void LineEdit::selection_delete()
{
	if (selection.enabled) {
		delete_text(selection.begin, selection.end);
	}

	deselect();
}

void LineEdit::set_max_length(int p_max_length)
{
	ERR_FAIL_COND(p_max_length < 0);
	max_length = p_max_length;
	set_text(text);
}

int LineEdit::get_max_length() const { return max_length; }

void LineEdit::selection_fill_at_caret()
{
	if (!selecting_enabled) {
		return;
	}

	selection.begin = caret_column;
	selection.end = selection.start_column;

	if (selection.end < selection.begin) {
		int aux = selection.end;
		selection.end = selection.begin;
		selection.begin = aux;
	}

	selection.enabled = (selection.begin != selection.end);
	queue_accessibility_update();
}

bool LineEdit::is_editable() const { return editable; }

bool LineEdit::is_secret() const { return pass; }

String LineEdit::get_secret_character() const { return secret_character; }

bool LineEdit::is_text_field() const { return true; }

void LineEdit::set_context_menu_enabled(bool p_enable) { context_menu_enabled = p_enable; }

bool LineEdit::is_context_menu_enabled() { return context_menu_enabled; }

void LineEdit::show_emoji_and_symbol_picker()
{
	_update_ime_window_position();
	DisplayServer::show_emoji_and_symbol_picker();
}

void LineEdit::set_emoji_menu_enabled(bool p_enabled)
{
	if (emoji_menu_enabled != p_enabled) {
		emoji_menu_enabled = p_enabled;
	}
}

bool LineEdit::is_emoji_menu_enabled() const { return emoji_menu_enabled; }

void LineEdit::set_backspace_deletes_composite_character_enabled(bool p_enabled)
{
	backspace_deletes_composite_character_enabled = p_enabled;
}

bool LineEdit::is_backspace_deletes_composite_character_enabled() const
{
	return backspace_deletes_composite_character_enabled;
}

bool LineEdit::is_menu_visible() const { return menu && menu->is_visible(); }

PopupMenu* LineEdit::get_menu() const
{
	if (!menu) {
		const_cast<LineEdit*>(this)->_generate_context_menu();
	}
	return menu;
}

bool LineEdit::is_expand_to_text_length_enabled() const { return expand_to_text_length; }

bool LineEdit::is_clear_button_enabled() const { return clear_button_enabled; }

void LineEdit::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool LineEdit::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

void LineEdit::set_virtual_keyboard_enabled(bool p_enable) { virtual_keyboard_enabled = p_enable; }

bool LineEdit::is_virtual_keyboard_enabled() const { return virtual_keyboard_enabled; }

void LineEdit::set_virtual_keyboard_show_on_focus(bool p_show_on_focus)
{
	virtual_keyboard_show_on_focus = p_show_on_focus;
}

bool LineEdit::get_virtual_keyboard_show_on_focus() const { return virtual_keyboard_show_on_focus; }

void LineEdit::set_virtual_keyboard_type(VirtualKeyboardType p_type)
{
	virtual_keyboard_type = p_type;
}

LineEdit::VirtualKeyboardType LineEdit::get_virtual_keyboard_type() const
{
	return virtual_keyboard_type;
}

void LineEdit::set_middle_mouse_paste_enabled(bool p_enabled)
{
	middle_mouse_paste_enabled = p_enabled;
}

bool LineEdit::is_middle_mouse_paste_enabled() const { return middle_mouse_paste_enabled; }

void LineEdit::set_selecting_enabled(bool p_enabled)
{
	if (selecting_enabled == p_enabled) {
		return;
	}

	selecting_enabled = p_enabled;

	if (!selecting_enabled) {
		deselect();
	}
}

bool LineEdit::is_selecting_enabled() const { return selecting_enabled; }

void LineEdit::set_deselect_on_focus_loss_enabled(const bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && selection.enabled && !has_focus()) {
		deselect();
	}
}

bool LineEdit::is_deselect_on_focus_loss_enabled() const { return deselect_on_focus_loss_enabled; }

void LineEdit::set_drag_and_drop_selection_enabled(const bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool LineEdit::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

Ref<Texture2D> LineEdit::get_right_icon() { return right_icon; }

LineEdit::ExpandMode LineEdit::get_icon_expand_mode() const { return icon_expand_mode; }

float LineEdit::get_right_icon_scale() const { return right_icon_scale; }

bool LineEdit::is_flat() const { return flat; }

void LineEdit::set_select_all_on_focus(bool p_enabled) { select_all_on_focus = p_enabled; }

bool LineEdit::is_select_all_on_focus() const { return select_all_on_focus; }

void LineEdit::clear_pending_select_all_on_focus() { pending_select_all_on_focus = false; }

void LineEdit::_text_changed()
{
	_emit_text_change();
	_clear_redo();
}

PackedStringArray LineEdit::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (secret_character.length() > 1) {
		warnings.push_back("Secret Character property supports only one character. Extra "
						   "characters will be ignored.");
	}
	return warnings;
}

void LineEdit::_fit_to_width()
{
	if (alignment == HORIZONTAL_ALIGNMENT_FILL) {
		Ref<StyleBox> style = theme_cache.normal;
		int t_width =
			get_size().width - style->get_margin(SIDE_RIGHT) - style->get_margin(SIDE_LEFT);
		bool using_placeholder = text.is_empty() && ime_text.is_empty();
		bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
		if (right_icon.is_valid() || display_clear_icon) {
			Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
			Point2 right_icon_size = _get_right_icon_size(r_icon);
			t_width -= right_icon_size.width;
		}
		TS->shaped_text_fit_to_width(text_rid, MAX(t_width, full_width));
	}
}

void LineEdit::_clear_redo()
{
	_create_undo_state();
	if (undo_stack_pos == nullptr) {
		return;
	}

	undo_stack_pos = undo_stack_pos->next();
	while (undo_stack_pos) {
		List<TextOperation>::Element* elem = undo_stack_pos;
		undo_stack_pos = undo_stack_pos->next();
		undo_stack.erase(elem);
	}
	_create_undo_state();
}

void LineEdit::_clear_undo_stack()
{
	undo_stack.clear();
	undo_stack_pos = nullptr;
	_create_undo_state();
}

void LineEdit::_create_undo_state()
{
	TextOperation op;
	op.text = text;
	op.caret_column = caret_column;
	op.scroll_offset = scroll_offset;
	undo_stack.push_back(op);
}

Key LineEdit::_get_menu_action_accelerator(const String& p_action)
{
	const List<InputEvent>* events = InputMap::action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<InputEvent>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const InputEvent& event = first_event->get();
	if (event.type != InputEventType::KEY) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero.
	if (event.key.physical_keycode != Key::NONE) {
		return event.get_physical_keycode_with_modifiers();
	}
	else {
		return event.get_keycode_with_modifiers();
	}
}

void LineEdit::_update_context_menu()
{
	if (!menu) {
		_generate_context_menu();
	}

	int idx = -1;

#define MENU_ITEM_ACTION_DISABLED(m_menu, m_id, m_action, m_disabled)                              \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_ACTION(m_menu, m_id, m_action)                                                   \
	idx = m_menu->get_item_index(m_id);                                                            \
                                                                                                   \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
	}

#define MENU_ITEM_DISABLED(m_menu, m_id, m_disabled)                                               \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_CHECKED(m_menu, m_id, m_checked)                                                 \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_checked(idx, m_checked);                                                  \
	}

	if (DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		MENU_ITEM_DISABLED(menu, MENU_EMOJI_AND_SYMBOL, !editable || !emoji_menu_enabled)
	}

#undef MENU_ITEM_ACTION_DISABLED
#undef MENU_ITEM_ACTION
#undef MENU_ITEM_DISABLED
#undef MENU_ITEM_CHECKED
}

void LineEdit::set_caret_column(int p_column) {}

LineEdit::LineEdit(const String& p_placeholder)
{
	text_rid = TS->create_shaped_text();
	_create_undo_state();
	deselect();
	set_focus_mode(FOCUS_ALL);
	set_default_cursor_shape(CURSOR_IBEAM);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_process_unhandled_key_input(true);
	set_caret_blink_enabled(false);
	set_placeholder(p_placeholder);
	set_editable(true); // Initialize to opposite first, so we get past the early-out in set_editable.
}

LineEdit::~LineEdit() { TS->free_rid(text_rid); }

void LineEdit::select_all() {}

void LineEdit::_emit_text_change() {}

void LineEdit::select(int p_from, int p_to) {}

void LineEdit::delete_text(int p_from_column, int p_to_column) {}

void LineEdit::_shape() {}


void LineEdit::set_right_icon(Ref<Texture2D> const&) {}

void LineEdit::set_placeholder(String) {}

void LineEdit::insert_text_at_caret(String) {}

void LineEdit::_set_text(String, bool) {}

void LineEdit::set_editable(bool) {}

void LineEdit::set_caret_blink_enabled(bool) {}

void LineEdit::_reset_caret_blink_timer() {}

void LineEdit::_generate_context_menu() {}

void LineEdit::_edit(bool, bool) {}

void LineEdit::set_secret(bool) {}

void LineEdit::set_horizontal_alignment(HorizontalAlignment) {}

void LineEdit::set_clear_button_enabled(bool) {}

void LineEdit::clear_internal() {}

String LineEdit::_get_accessibility_name() const { return String(); }

// --- line_edit.compat.inc ---

#ifndef DISABLE_DEPRECATED


void LineEdit::_edit_bind_compat_111117() { edit(false); }

void LineEdit::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- item_list.cpp ---

String ItemList::get_item_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].text;
}

Control::TextDirection ItemList::get_item_text_direction(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), TEXT_DIRECTION_INHERITED);
	return items[p_idx].text_direction;
}

String ItemList::get_item_language(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].language;
}

Node::AutoTranslateMode ItemList::get_item_auto_translate_mode(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), AUTO_TRANSLATE_MODE_INHERIT);
	return items[p_idx].auto_translate_mode;
}

void ItemList::set_item_tooltip_enabled(int p_idx, const bool p_enabled)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].tooltip_enabled != p_enabled) {
		items.write[p_idx].tooltip_enabled = p_enabled;
		items.write[p_idx].accessibility_item_dirty = true;
		queue_accessibility_update();
	}
}

bool ItemList::is_item_tooltip_enabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].tooltip_enabled;
}

String ItemList::get_item_tooltip(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].tooltip;
}

Ref<Texture2D> ItemList::get_item_icon(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());

	return items[p_idx].icon;
}

bool ItemList::is_item_icon_transposed(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].icon_transposed;
}

Rect2 ItemList::get_item_icon_region(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	return items[p_idx].icon_region;
}

Color ItemList::get_item_icon_modulate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].icon_modulate;
}

Color ItemList::get_item_custom_bg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_bg;
}

Color ItemList::get_item_custom_fg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_fg;
}

Rect2 ItemList::get_item_rect(int p_idx, bool p_expand) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	Rect2 ret = items[p_idx].rect_cache;
	if (p_expand && p_idx % current_columns == current_columns - 1) {
		int width = get_size().width - theme_cache.panel_style->get_minimum_size().width;
		if (scroll_bar_v->is_visible()) {
			width -=
				scroll_bar_v->get_bound_minimum_size().width + theme_cache.scroll_bar_h_separation;
		}
		ret.size.width = width - ret.position.x;
	}
	ret.position += theme_cache.panel_style->get_offset();
	return ret;
}

void ItemList::set_item_selectable(int p_idx, bool p_selectable)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	items.write[p_idx].selectable = p_selectable;
	items.write[p_idx].accessibility_item_dirty = true;
	queue_accessibility_update();
}

bool ItemList::is_item_selectable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].selectable;
}

bool ItemList::is_item_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

bool ItemList::is_selected(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].selected;
}

int ItemList::get_current() const { return current; }

int ItemList::get_item_count() const { return items.size(); }

int ItemList::get_fixed_column_width() const { return fixed_column_width; }

bool ItemList::is_same_column_width() const { return same_column_width; }

int ItemList::get_max_text_lines() const { return max_text_lines; }

int ItemList::get_max_columns() const { return max_columns; }

ItemList::SelectMode ItemList::get_select_mode() const { return select_mode; }

ItemList::IconMode ItemList::get_icon_mode() const { return icon_mode; }

Size2i ItemList::get_fixed_icon_size() const { return fixed_icon_size; }

Size2 ItemList::Item::get_icon_size() const
{
	if (icon.is_null()) {
		return Size2();
	}

	Size2 size_result = Size2(icon_region.size).abs();
	if (icon_region.size.x == 0 || icon_region.size.y == 0) {
		size_result = icon->get_size();
	}

	if (icon_transposed) {
		Size2 size_tmp = size_result;
		size_result.x = size_tmp.y;
		size_result.y = size_tmp.x;
	}

	return size_result;
}

void ItemList::center_on_current(bool p_center_verically, bool p_center_horizontally)
{
	if (current < 0 || current >= items.size()) {
		return;
	}

	ERR_FAIL_COND_MSG(!p_center_verically && !p_center_horizontally,
		"At least one of the parameters must be true.");

	Rect2 r = items[current].rect_cache;

	if (p_center_verically) {
		int from_v = scroll_bar_v->get_value();
		int to_v = from_v + scroll_bar_v->get_page();
		int item_center_y = r.position.y + r.size.y / 2;
		int viewport_center_y = (from_v + to_v) / 2;

		int offset_y = item_center_y - viewport_center_y;
		scroll_bar_v->set_value(scroll_bar_v->get_value() + offset_y);
	}

	if (p_center_horizontally) {
		int from_h = scroll_bar_h->get_value();
		int to_h = from_h + scroll_bar_h->get_page();
		int item_center_x = r.position.x + r.size.x / 2;
		int viewport_center_x = (from_h + to_h) / 2;

		int offset_x = item_center_x - viewport_center_x;
		scroll_bar_h->set_value(scroll_bar_h->get_value() + offset_x);
	}
}

static Rect2 _adjust_to_max_size(Size2 p_size, Size2 p_max_size)
{
	Size2 size = p_max_size;
	int tex_width = p_size.width * size.height / p_size.height;
	int tex_height = size.height;

	if (tex_width > size.width) {
		tex_width = size.width;
		tex_height = p_size.height * tex_width / p_size.width;
	}

	int ofs_x = (size.width - tex_width) / 2;
	int ofs_y = (size.height - tex_height) / 2;

	return Rect2(ofs_x, ofs_y, tex_width, tex_height);
}

RID ItemList::get_focused_accessibility_element() const
{
	if (current == -1) {
		return get_accessibility_element();
	}
	else {
		const Item& item = items[current];
		return item.accessibility_item_element;
	}
}

int ItemList::get_item_at_position(const Point2& p_pos, bool p_exact) const
{
	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();
	pos.x += scroll_bar_h->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - scroll_bar_h->get_value() -
				theme_cache.panel_style->get_margin(SIDE_LEFT) -
				theme_cache.panel_style->get_margin(SIDE_RIGHT);
	}

	int closest = -1;
	int closest_dist = 0x7FFFFFFF;

	for (int i = 0; i < items.size(); i++) {
		Rect2 rc = items[i].rect_cache;

		if (i % current_columns ==
			current_columns -
				1) { // Make sure you can still select the last item when clicking past the column.
			if (is_layout_rtl()) {
				rc.size.width = get_size().width - scroll_bar_h->get_value() + rc.position.x;
			}
			else {
				rc.size.width = get_size().width + scroll_bar_h->get_value() - rc.position.x;
			}
		}

		if (rc.size.x < 0) {
			continue; // Skip negative item sizes, because they are off screen.
		}

		if (rc.has_point(pos)) {
			closest = i;

			break;
		}

		float dist = rc.distance_to(pos);
		if (!p_exact && dist < closest_dist) {
			closest = i;
			closest_dist = dist;
		}
	}

	return closest;
}

bool ItemList::is_pos_at_end_of_items(const Point2& p_pos) const
{
	if (items.is_empty()) {
		return true;
	}

	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x;
	}

	Rect2 endrect = items[items.size() - 1].rect_cache;
	return (pos.y > endrect.position.y + endrect.size.y);
}

Node::AutoTranslateMode ItemList::get_tooltip_auto_translate_mode_at(const Point2& p_at) const
{
	int closest = get_item_at_position(p_at, true);
	if (closest != -1) {
		return items[closest].auto_translate_mode;
	}
	return Control::get_tooltip_auto_translate_mode_at(p_at);
}

void ItemList::set_allow_rmb_select(bool p_allow) { allow_rmb_select = p_allow; }

bool ItemList::get_allow_rmb_select() const { return allow_rmb_select; }

void ItemList::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool ItemList::get_allow_reselect() const { return allow_reselect; }

void ItemList::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool ItemList::get_allow_search() const { return allow_search; }

real_t ItemList::get_icon_scale() const { return icon_scale; }

Vector<int> ItemList::get_selected_items()
{
	Vector<int> selected;
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			selected.push_back(i);
			if (select_mode == SELECT_SINGLE) {
				break;
			}
		}
	}
	return selected;
}

bool ItemList::is_anything_selected()
{
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			return true;
		}
	}

	return false;
}

Size2 ItemList::get_minimum_size() const
{
	Size2 min_size;
	if (auto_width) {
		min_size.x = auto_width_value;
	}

	if (auto_height) {
		min_size.y = auto_height_value;
	}
	return min_size;
}

void ItemList::set_autoscroll_to_bottom(const bool p_enable) { do_autoscroll_to_bottom = p_enable; }

bool ItemList::has_auto_width() const { return auto_width; }

bool ItemList::has_auto_height() const { return auto_height; }

TextServer::OverrunBehavior ItemList::get_text_overrun_behavior() const
{
	return text_overrun_behavior;
}

bool ItemList::has_wraparound_items() const { return wraparound_items; }

ItemList::ScrollHintMode ItemList::get_scroll_hint_mode() const { return scroll_hint_mode; }

bool ItemList::is_scroll_hint_tiled() { return tile_scroll_hint; }

ItemList::~ItemList() {}

void ItemList::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon) {}

void ItemList::deselect_all() {}


void ItemList::set_scroll_hint_mode(ItemList::ScrollHintMode) {}

void ItemList::set_fixed_column_width(int) {}

void ItemList::set_fixed_icon_size(Vector2i const&) {}

void ItemList::select(int, bool) {}

void ItemList::set_current(int) {}

void ItemList::ensure_current_is_visible() {}

void ItemList::clear() {}

int ItemList::add_item(String const&, Ref<Texture2D> const&, bool) {}

void ItemList::set_icon_scale(float) {}

ItemList::ItemList() {}

// --- texture_rect.cpp ---

Size2 TextureRect::get_minimum_size() const
{
	if (texture.is_valid()) {
		switch (expand_mode) {
		case EXPAND_KEEP_SIZE: {
			return texture->get_size();
		} break;
		case EXPAND_IGNORE_SIZE: {
			return Size2();
		} break;
		case EXPAND_FIT_WIDTH: {
			return Size2(get_size().y, 0);
		} break;
		case EXPAND_FIT_WIDTH_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_width()) / texture->get_height();
			return Size2(get_size().y * ratio, 0);
		} break;
		case EXPAND_FIT_HEIGHT: {
			return Size2(0, get_size().x);
		} break;
		case EXPAND_FIT_HEIGHT_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_height()) / texture->get_width();
			return Size2(0, get_size().x * ratio);
		} break;
		}
	}
	return Size2();
}

PackedStringArray TextureRect::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (stretch_mode == STRETCH_TILE) {
		Ref<AtlasTexture> at = texture;
		while (at.is_valid() && !at->is_animated()) {
			if (at->get_margin() != Rect2()) {
				warnings.push_back(vformat(RTR("STRETCH_TILE mode is not supported for an "
											   "AtlasTexture with non-zero margin.")));
				break;
			}
			at = at->get_atlas();
		}
	}

	return warnings;
}

Ref<Texture2D> TextureRect::get_texture() const { return texture; }

TextureRect::ExpandMode TextureRect::get_expand_mode() const { return expand_mode; }

void TextureRect::set_expand_mode(ExpandMode p_mode) {}

TextureRect::StretchMode TextureRect::get_stretch_mode() const { return stretch_mode; }

bool TextureRect::is_flipped_h() const { return hflip; }

bool TextureRect::is_flipped_v() const { return vflip; }

TextureRect::TextureRect() { set_mouse_filter(MOUSE_FILTER_PASS); }

TextureRect::~TextureRect() {}

void TextureRect::set_texture(const Ref<Texture2D>& p_tex) {}

void TextureRect::set_stretch_mode(StretchMode p_mode) {}

// --- nine_patch_rect.cpp ---

void NinePatchRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (texture.is_null()) {
			return;
		}

		Rect2 rect = Rect2(Point2(), get_size());
		Rect2 src_rect = region_rect;

		texture->get_rect_region(rect, src_rect, rect, src_rect);

		RID ci = get_canvas_item();
		Renderer::canvas_item_add_nine_patch(ci, rect, src_rect,
			texture->get_scaled_rid(), Vector2(margin[SIDE_LEFT], margin[SIDE_TOP]),
			Vector2(margin[SIDE_RIGHT], margin[SIDE_BOTTOM]), RSE::NinePatchAxisMode(axis_h),
			RSE::NinePatchAxisMode(axis_v), draw_center);
	} break;
	}
}

Size2 NinePatchRect::get_minimum_size() const
{
	return Size2(margin[SIDE_LEFT] + margin[SIDE_RIGHT], margin[SIDE_TOP] + margin[SIDE_BOTTOM]);
}

Ref<Texture2D> NinePatchRect::get_texture() const { return texture; }

int NinePatchRect::get_patch_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return margin[p_side];
}

void NinePatchRect::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;

	item_rect_changed();
}

Rect2 NinePatchRect::get_region_rect() const { return region_rect; }

bool NinePatchRect::is_draw_center_enabled() const { return draw_center; }

NinePatchRect::AxisStretchMode NinePatchRect::get_h_axis_stretch_mode() const { return axis_h; }

NinePatchRect::AxisStretchMode NinePatchRect::get_v_axis_stretch_mode() const { return axis_v; }

NinePatchRect::NinePatchRect() { set_mouse_filter(MOUSE_FILTER_IGNORE); }

NinePatchRect::~NinePatchRect() {}

// --- reference_rect.cpp ---

void ReferenceRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!is_inside_tree()) {
			return;
		}
		if (Engine::is_editor_hint() || !editor_only) {
			draw_rect(Rect2(Point2(), get_size()), border_color, false, border_width);
		}
	} break;
	}
}

Color ReferenceRect::get_border_color() const { return border_color; }

float ReferenceRect::get_border_width() const { return border_width; }

bool ReferenceRect::get_editor_only() const { return editor_only; }

// --- video_stream_player.cpp ---

int VideoStreamPlayer::sp_get_channel_count() const
{
	if (playback.is_null()) {
		return 0;
	}

	return playback->get_channels();
}

bool VideoStreamPlayer::mix(AudioFrame* p_buffer, int p_frames)
{
	// Check the amount resampler can really handle.
	// If it cannot, wait "wait_resampler_phase_limit" times.
	// This mechanism contributes to smoother pause/unpause operation.
	if (p_frames <= resampler.get_num_of_ready_frames() || wait_resampler_limit <= wait_resampler) {
		wait_resampler = 0;
		return resampler.mix(p_buffer, p_frames);
	}
	wait_resampler++;
	return false;
}

// Called from main thread (e.g. VideoStreamPlayback::update).
int VideoStreamPlayer::_audio_mix_callback(void* p_udata, const float* p_data, int p_frames)
{
	ERR_FAIL_NULL_V(p_udata, 0);
	ERR_FAIL_NULL_V(p_data, 0);

	VideoStreamPlayer* vp = static_cast<VideoStreamPlayer*>(p_udata);

	int todo = MIN(vp->resampler.get_writer_space(), p_frames);

	float* wb = vp->resampler.get_write_buffer();
	int c = vp->resampler.get_channel_count();

	for (int i = 0; i < todo * c; i++) {
		wb[i] = p_data[i];
	}
	vp->resampler.write(todo);

	return todo;
}

void VideoStreamPlayer::_mix_audios(void* p_self)
{
	ERR_FAIL_NULL(p_self);
	static_cast<VideoStreamPlayer*>(p_self)->_mix_audio();
}

// Called from audio thread
void VideoStreamPlayer::_mix_audio()
{
	if (stream.is_null()) {
		return;
	}
	if (playback.is_null() || !playback->is_playing() || playback->is_paused()) {
		return;
	}

	AudioFrame* buffer = mix_buffer.ptrw();
	int buffer_size = mix_buffer.size();

	// Resample
	if (!mix(buffer, buffer_size)) {
		return;
	}

	AudioFrame vol = AudioFrame(volume, volume);

	int cc = AudioServer::get_channel_count();

	if (cc == 1) {
		AudioFrame* target =
			AudioServer::thread_get_channel_mix_buffer(bus_index, 0);
		ERR_FAIL_NULL(target);

		for (int j = 0; j < buffer_size; j++) {
			target[j] += buffer[j] * vol;
		}

	}
	else {
		AudioFrame* targets[4];

		for (int k = 0; k < cc; k++) {
			targets[k] = AudioServer::thread_get_channel_mix_buffer(bus_index, k);
			ERR_FAIL_NULL(targets[k]);
		}

		for (int j = 0; j < buffer_size; j++) {
			AudioFrame frame = buffer[j] * vol;
			for (int k = 0; k < cc; k++) {
				targets[k][j] += frame;
			}
		}
	}
}

Size2 VideoStreamPlayer::get_minimum_size() const
{
	if (!expand && texture.is_valid()) {
		return texture_size;
	}
	else {
		return Size2();
	}
}

bool VideoStreamPlayer::has_expand() const { return expand; }

void VideoStreamPlayer::set_loop(bool p_loop) { loop = p_loop; }

bool VideoStreamPlayer::has_loop() const { return loop; }

Ref<VideoStream> VideoStreamPlayer::get_stream() const { return stream; }

void VideoStreamPlayer::play()
{
	ERR_FAIL_COND(!is_inside_tree());
	if (playback.is_null()) {
		return;
	}
	playback->play();
	set_process_internal(true);
	first_frame = true;

	if (!can_process()) {
		_notification(NOTIFICATION_PAUSED);
	}
}

void VideoStreamPlayer::stop()
{
	if (!is_inside_tree()) {
		return;
	}
	if (playback.is_null()) {
		return;
	}

	playback->stop();
	resampler.flush();
	set_process_internal(false);
}

bool VideoStreamPlayer::is_playing() const
{
	if (playback.is_null()) {
		return false;
	}

	return playback->is_playing();
}

void VideoStreamPlayer::set_paused(bool p_paused)
{
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;
	if (!p_paused && !can_process()) {
		paused_from_tree = true;
		return;
	}
	else if (p_paused && paused_from_tree) {
		paused_from_tree = false;
		return;
	}

	if (playback.is_valid()) {
		playback->set_paused(p_paused);
		set_process_internal(!p_paused);
	}
}

bool VideoStreamPlayer::is_paused() const { return paused; }

void VideoStreamPlayer::set_buffering_msec(int p_msec) { buffering_ms = p_msec; }

int VideoStreamPlayer::get_buffering_msec() const { return buffering_ms; }

void VideoStreamPlayer::set_audio_track(int p_track)
{
	audio_track = p_track;
	if (stream.is_valid()) {
		stream->set_audio_track(audio_track);
	}
	if (playback.is_valid()) {
		playback->set_audio_track(audio_track);
	}
}

int VideoStreamPlayer::get_audio_track() const { return audio_track; }

void VideoStreamPlayer::set_volume(float p_vol) { volume = p_vol; }

float VideoStreamPlayer::get_volume() const { return volume; }

void VideoStreamPlayer::set_volume_db(float p_db)
{
	if (p_db < -79) {
		set_volume(0);
	}
	else {
		set_volume(Math::db_to_linear(p_db));
	}
}

float VideoStreamPlayer::get_volume_db() const
{
	if (volume == 0) {
		return -80;
	}
	else {
		return Math::linear_to_db(volume);
	}
}

void VideoStreamPlayer::set_speed_scale(float p_speed_scale)
{
	ERR_FAIL_COND(p_speed_scale < 0.0);
	speed_scale = p_speed_scale;
}

float VideoStreamPlayer::get_speed_scale() const { return speed_scale; }

String VideoStreamPlayer::get_stream_name() const
{
	if (stream.is_null()) {
		return "<No Stream>";
	}
	return stream->get_name();
}

double VideoStreamPlayer::get_stream_length() const
{
	if (playback.is_null()) {
		return 0;
	}
	return playback->get_length();
}

double VideoStreamPlayer::get_stream_position() const
{
	if (playback.is_null()) {
		return 0;
	}
	return playback->get_playback_position();
}

void VideoStreamPlayer::set_stream_position(double p_position)
{
	if (playback.is_valid()) {
		resampler.flush();
		playback->seek(p_position);
		first_frame = true;
	}
}

Ref<Texture2D> VideoStreamPlayer::get_video_texture() const
{
	if (playback.is_valid()) {
		return playback->get_texture();
	}

	return Ref<Texture2D>();
}

void VideoStreamPlayer::set_autoplay(bool p_enable) { autoplay = p_enable; }

bool VideoStreamPlayer::has_autoplay() const { return autoplay; }

void VideoStreamPlayer::set_bus(const String& p_bus)
{
	// If audio is active, must lock this.
	AudioServer::lock();
	bus = p_bus;
	AudioServer::unlock();
}

String VideoStreamPlayer::get_bus() const
{
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == bus) {
			return bus;
		}
	}
	return SceneString(Master);
}

VideoStreamPlayer::~VideoStreamPlayer()
{
	// Not necessary here, but make in consistent with other "stream_player" classes.
	resampler.clear();
}



void VideoStreamPlayer::_notification(int) {}

// --- virtual_joystick.cpp ---

void VirtualJoystick::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!Engine::is_editor_hint() && visibility == VISIBILITY_WHEN_TOUCHED &&
			!is_pressed) {
			return;
		}

		Rect2 rect_joystick = Rect2(joystick_pos - Vector2(0.5, 0.5) * joystick_size,
			Vector2(joystick_size, joystick_size));
		draw_style_box(
			is_pressed ? theme_cache.pressed_joystick.ptr() : theme_cache.normal_joystick.ptr(),
			rect_joystick);

		Rect2 rect_tip = Rect2(tip_pos - Vector2(0.5, 0.5) * tip_size, Vector2(tip_size, tip_size));
		draw_style_box(
			is_pressed ? theme_cache.pressed_tip.ptr() : theme_cache.normal_tip.ptr(), rect_tip);
	} break;

	case NOTIFICATION_ENTER_TREE: {
		joystick_pos = get_size() * initial_offset_ratio;
		tip_pos = joystick_pos;
	} break;

	case NOTIFICATION_RESIZED: {
		_reset();
	} break;
	}
}

void VirtualJoystick::_reset() {}

void VirtualJoystick::_handle_input_actions()
{
	if (input_vector.x >= 0.0f && Input::is_action_pressed(action_left)) {
		Input::action_release(action_left);
	}
	if (input_vector.x <= 0.0f && Input::is_action_pressed(action_right)) {
		Input::action_release(action_right);
	}
	if (input_vector.y >= 0.0f && Input::is_action_pressed(action_up)) {
		Input::action_release(action_up);
	}
	if (input_vector.y <= 0.0f && Input::is_action_pressed(action_down)) {
		Input::action_release(action_down);
	}

	if (input_vector.x < 0.0f) {
		Input::action_press(action_left, -input_vector.x);
	}
	else if (input_vector.x > 0.0f) {
		Input::action_press(action_right, input_vector.x);
	}
	if (input_vector.y < 0.0f) {
		Input::action_press(action_up, -input_vector.y);
	}
	else if (input_vector.y > 0.0f) {
		Input::action_press(action_down, input_vector.y);
	}
}

Vector2 VirtualJoystick::get_joystick_position() const { return joystick_pos; }

void VirtualJoystick::set_joystick_size(float p_size)
{
	if (joystick_size == p_size) {
		return;
	}
	joystick_size = p_size;
	_reset();
}

float VirtualJoystick::get_joystick_size() const { return joystick_size; }

void VirtualJoystick::set_tip_size(float p_size)
{
	if (tip_size == p_size) {
		return;
	}
	tip_size = p_size;
	_reset();
}

float VirtualJoystick::get_tip_size() const { return tip_size; }

float VirtualJoystick::get_deadzone_ratio() const { return deadzone_ratio; }

float VirtualJoystick::get_clampzone_ratio() const { return clampzone_ratio; }

void VirtualJoystick::set_initial_offset_ratio(const Vector2& p_ratio)
{
	if (initial_offset_ratio == p_ratio) {
		return;
	}
	initial_offset_ratio = p_ratio;
	_reset();
}

Vector2 VirtualJoystick::get_initial_offset_ratio() const { return initial_offset_ratio; }

void VirtualJoystick::set_joystick_mode(JoystickMode p_mode) { joystick_mode = p_mode; }

VirtualJoystick::JoystickMode VirtualJoystick::get_joystick_mode() const { return joystick_mode; }

void VirtualJoystick::set_action_left(const String& p_action) { action_left = p_action; }

String VirtualJoystick::get_action_left() const { return action_left; }

void VirtualJoystick::set_action_right(const String& p_action) { action_right = p_action; }

String VirtualJoystick::get_action_right() const { return action_right; }

void VirtualJoystick::set_action_up(const String& p_action) { action_up = p_action; }

String VirtualJoystick::get_action_up() const { return action_up; }

void VirtualJoystick::set_action_down(const String& p_action) { action_down = p_action; }

String VirtualJoystick::get_action_down() const { return action_down; }

void VirtualJoystick::set_visibility_mode(VisibilityMode p_mode) { visibility = p_mode; }

VirtualJoystick::VisibilityMode VirtualJoystick::get_visibility_mode() const { return visibility; }
