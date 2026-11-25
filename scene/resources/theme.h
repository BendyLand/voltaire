/**************************************************************************/
/*  theme.h                                                               */
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

#include "core/types.h"
// --- Folded from scene/resources/font.h ---
#include "core/types.h"
// --- Folded from servers/text/text_server.h ---
#include "core/io/resource_loader.h"
#include "core/math/vector2.h"
#include "core/types.h"

template <typename T> class TypedArray;

struct Glyph;
struct CaretInfo;

#define OT_TAG(m_c1, m_c2, m_c3, m_c4)                                                             \
	((int32_t)((((uint32_t)(m_c1)&0xff) << 24) | (((uint32_t)(m_c2)&0xff) << 16) |                 \
			   (((uint32_t)(m_c3)&0xff) << 8) | ((uint32_t)(m_c4)&0xff)))

class TextServer : public RefCounted
{
public:
	enum FontAntialiasing
	{
		FONT_ANTIALIASING_NONE,
		FONT_ANTIALIASING_GRAY,
		FONT_ANTIALIASING_LCD,
	};

	enum FontLCDSubpixelLayout
	{
		FONT_LCD_SUBPIXEL_LAYOUT_NONE,
		FONT_LCD_SUBPIXEL_LAYOUT_HRGB,
		FONT_LCD_SUBPIXEL_LAYOUT_HBGR,
		FONT_LCD_SUBPIXEL_LAYOUT_VRGB,
		FONT_LCD_SUBPIXEL_LAYOUT_VBGR,
		FONT_LCD_SUBPIXEL_LAYOUT_MAX,
	};

	enum Direction
	{
		DIRECTION_AUTO,
		DIRECTION_LTR,
		DIRECTION_RTL,
		DIRECTION_INHERITED,
	};

	enum Orientation
	{
		ORIENTATION_HORIZONTAL,
		ORIENTATION_VERTICAL
	};

	enum JustificationFlag
	{
		JUSTIFICATION_NONE = 0,
		JUSTIFICATION_KASHIDA = 1 << 0,
		JUSTIFICATION_WORD_BOUND = 1 << 1,
		JUSTIFICATION_TRIM_EDGE_SPACES = 1 << 2,
		JUSTIFICATION_AFTER_LAST_TAB = 1 << 3,
		JUSTIFICATION_CONSTRAIN_ELLIPSIS = 1 << 4,
		JUSTIFICATION_SKIP_LAST_LINE = 1 << 5,
		JUSTIFICATION_SKIP_LAST_LINE_WITH_VISIBLE_CHARS = 1 << 6,
		JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE = 1 << 7,
	};

	enum VisibleCharactersBehavior
	{
		VC_CHARS_BEFORE_SHAPING,
		VC_CHARS_AFTER_SHAPING,
		VC_GLYPHS_AUTO,
		VC_GLYPHS_LTR,
		VC_GLYPHS_RTL,
	};

	enum AutowrapMode
	{
		AUTOWRAP_OFF,
		AUTOWRAP_ARBITRARY,
		AUTOWRAP_WORD,
		AUTOWRAP_WORD_SMART
	};

	enum LineBreakFlag
	{
		BREAK_NONE = 0,
		BREAK_MANDATORY = 1 << 0,
		BREAK_WORD_BOUND = 1 << 1,
		BREAK_GRAPHEME_BOUND = 1 << 2,
		BREAK_ADAPTIVE = 1 << 3,
#ifndef DISABLE_DEPRECATED
		BREAK_TRIM_EDGE_SPACES = 1 << 4,
#else
	// RESERVED = 1 << 4,
#endif
		BREAK_TRIM_INDENT = 1 << 5,
		BREAK_TRIM_START_EDGE_SPACES = 1 << 6,
		BREAK_TRIM_END_EDGE_SPACES = 1 << 7,

		BREAK_TRIM_MASK =
			BREAK_TRIM_INDENT | BREAK_TRIM_START_EDGE_SPACES | BREAK_TRIM_END_EDGE_SPACES,
	};

	enum OverrunBehavior
	{
		OVERRUN_NO_TRIMMING,
		OVERRUN_TRIM_CHAR,
		OVERRUN_TRIM_WORD,
		OVERRUN_TRIM_ELLIPSIS,
		OVERRUN_TRIM_WORD_ELLIPSIS,
		OVERRUN_TRIM_ELLIPSIS_FORCE,
		OVERRUN_TRIM_WORD_ELLIPSIS_FORCE,
	};

	enum TextOverrunFlag
	{
		OVERRUN_NO_TRIM = 0,
		OVERRUN_TRIM = 1 << 0,
		OVERRUN_TRIM_WORD_ONLY = 1 << 1,
		OVERRUN_ADD_ELLIPSIS = 1 << 2,
		OVERRUN_ENFORCE_ELLIPSIS = 1 << 3,
		OVERRUN_JUSTIFICATION_AWARE = 1 << 4,
		OVERRUN_SHORT_STRING_ELLIPSIS = 1 << 5,
	};

	enum GraphemeFlag
	{
		GRAPHEME_IS_VALID = 1 << 0,		 // Grapheme is valid.
		GRAPHEME_IS_RTL = 1 << 1,		 // Grapheme is right-to-left.
		GRAPHEME_IS_VIRTUAL = 1 << 2,	 // Grapheme is not part of source string (added by
										 // fit_to_width function, do not affect caret movement).
		GRAPHEME_IS_SPACE = 1 << 3,		 // Is whitespace (for justification and word breaks).
		GRAPHEME_IS_BREAK_HARD = 1 << 4, // Is line break (mandatory break, e.g. "\n").
		GRAPHEME_IS_BREAK_SOFT = 1 << 5, // Is line break (optional break, e.g. space).
		GRAPHEME_IS_TAB = 1 << 6,		 // Is tab or vertical tab.
		GRAPHEME_IS_ELONGATION = 1 << 7, // Elongation (e.g. kashida), grapheme can be duplicated or
										 // truncated to fit line to width.
		GRAPHEME_IS_PUNCTUATION = 1 << 8, // Punctuation, except underscore (can be used as word
										  // break, but not line break or justifiction).
		GRAPHEME_IS_UNDERSCORE = 1 << 9,  // Underscore (can be used as word break).
		GRAPHEME_IS_CONNECTED = 1 << 10,  // Connected to previous grapheme.
		GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL =
			1 << 11, // It is safe to insert a U+0640 before this grapheme for elongation.
		GRAPHEME_IS_EMBEDDED_OBJECT =
			1 << 12, // Grapheme is an object replacement character for the embedded object.
		GRAPHEME_IS_SOFT_HYPHEN = 1 << 13, // Grapheme is a soft hyphen.
	};

	enum Hinting
	{
		HINTING_NONE,
		HINTING_LIGHT,
		HINTING_NORMAL
	};

	enum SubpixelPositioning
	{
		SUBPIXEL_POSITIONING_DISABLED = 0,
		SUBPIXEL_POSITIONING_AUTO = 1,
		SUBPIXEL_POSITIONING_ONE_HALF = 2,
		SUBPIXEL_POSITIONING_ONE_QUARTER = 3,

		SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE = 20,
		SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE = 16,
	};

	enum Feature
	{
		FEATURE_SIMPLE_LAYOUT = 1 << 0,
		FEATURE_BIDI_LAYOUT = 1 << 1,
		FEATURE_VERTICAL_LAYOUT = 1 << 2,
		FEATURE_SHAPING = 1 << 3,
		FEATURE_KASHIDA_JUSTIFICATION = 1 << 4,
		FEATURE_BREAK_ITERATORS = 1 << 5,
		FEATURE_FONT_BITMAP = 1 << 6,
		FEATURE_FONT_DYNAMIC = 1 << 7,
		FEATURE_FONT_MSDF = 1 << 8,
		FEATURE_FONT_SYSTEM = 1 << 9,
		FEATURE_FONT_VARIABLE = 1 << 10,
		FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION = 1 << 11,
		FEATURE_USE_SUPPORT_DATA = 1 << 12,
		FEATURE_UNICODE_IDENTIFIERS = 1 << 13,
		FEATURE_UNICODE_SECURITY = 1 << 14,
	};

	enum ContourPointTag
	{
		CONTOUR_CURVE_TAG_ON = 0x01,
		CONTOUR_CURVE_TAG_OFF_CONIC = 0x00,
		CONTOUR_CURVE_TAG_OFF_CUBIC = 0x02
	};

	enum SpacingType
	{
		SPACING_GLYPH,
		SPACING_SPACE,
		SPACING_TOP,
		SPACING_BOTTOM,
		SPACING_MAX,
	};

	enum FontStyle
	{
		FONT_BOLD = 1 << 0,
		FONT_ITALIC = 1 << 1,
		FONT_FIXED_WIDTH = 1 << 2,
	};

	enum StructuredTextParser
	{
		STRUCTURED_TEXT_DEFAULT,
		STRUCTURED_TEXT_URI,
		STRUCTURED_TEXT_FILE,
		STRUCTURED_TEXT_EMAIL,
		STRUCTURED_TEXT_LIST,
		STRUCTURED_TEXT_GDSCRIPT,
		STRUCTURED_TEXT_CUSTOM
	};

	enum FixedSizeScaleMode
	{
		FIXED_SIZE_SCALE_DISABLE,
		FIXED_SIZE_SCALE_INTEGER_ONLY,
		FIXED_SIZE_SCALE_ENABLED,
	};

	void _draw_hex_code_box_number(const RID& p_canvas, int64_t p_size, const Vector2& p_pos,
		uint8_t p_index, const Color& p_color) const;

protected:
	static double vp_oversampling;
	HashMap<char32_t, char32_t> diacritics_map;
	void _diacritics_map_add(const String& p_from, char32_t p_to);
	void _init_diacritics_map();


#ifndef DISABLE_DEPRECATED
	void _font_draw_glyph_bind_compat_104872(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1)) const;
	void _font_draw_glyph_outline_bind_compat_104872(const RID& p_font, const RID& p_canvas,
		int64_t p_size, int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1)) const;
	void _shaped_text_draw_bind_compat_104872(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		const Color& p_color = Color(1, 1, 1)) const;
	void _shaped_text_draw_outline_bind_compat_104872(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		int64_t p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	PackedInt32Array _shaped_text_get_word_breaks_bind_compat_90732(const RID& p_shaped,
		uint32_t p_grapheme_flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_PUNCTUATION) const;
	static void _bind_compatibility_methods();
#endif

public:
	static uint32_t get_overrun_flags_from_behavior(OverrunBehavior p_behavior);

	virtual bool has_feature(Feature p_feature) const { return false; }

	virtual String get_name() const { return String(); }

	virtual String get_short_name() const { return String(); }

	virtual int64_t get_features() const { return 0; }

	virtual void free_rid(const RID& p_rid) {}

	virtual bool has(const RID& p_rid) { return false; }

	virtual bool load_support_data(const String& p_filename) { return false; }

	virtual String get_support_data_filename() const { return String(); }

	virtual String get_support_data_info() const { return String(); }

	virtual bool save_support_data(const String& p_filename) const { return false; }

	virtual PackedByteArray get_support_data() const { return PackedByteArray(); }

	virtual bool is_locale_using_support_data(const String& p_locale) const { return false; }

	virtual bool is_locale_right_to_left(const String& p_locale) const { return false; }

	virtual int64_t name_to_tag(const String& p_name) const;
	virtual String tag_to_name(int64_t p_tag) const;

	/* Font interface */

	virtual RID create_font() { return RID(); }

	virtual RID create_font_linked_variation(const RID& p_font_rid) { return RID(); }

	virtual void font_set_data(const RID& p_font_rid, const PackedByteArray& p_data) {}

	virtual void font_set_data_ptr(
		const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size)
	{
	}

	virtual void font_set_face_index(const RID& p_font_rid, int64_t p_index) {}

	virtual int64_t font_get_face_index(const RID& p_font_rid) const { return 0; }

	virtual int64_t font_get_face_count(const RID& p_font_rid) const { return 0; }

	virtual void font_set_style(const RID& p_font_rid, uint32_t p_style) {}

	virtual uint32_t font_get_style(const RID& p_font_rid) const { return 0; }

	virtual void font_set_name(const RID& p_font_rid, const String& p_name) {}

	virtual String font_get_name(const RID& p_font_rid) const { return String(); }

	virtual void font_set_style_name(const RID& p_font_rid, const String& p_name) {}

	virtual String font_get_style_name(const RID& p_font_rid) const { return String(); }

	virtual void font_set_weight(const RID& p_font_rid, int64_t p_weight) {}

	virtual int64_t font_get_weight(const RID& p_font_rid) const { return 0; }

	virtual void font_set_stretch(const RID& p_font_rid, int64_t p_stretch) {}

	virtual int64_t font_get_stretch(const RID& p_font_rid) const { return 0; }

	virtual void font_set_antialiasing(const RID& p_font_rid, FontAntialiasing p_antialiasing) {}

	virtual FontAntialiasing font_get_antialiasing(const RID& p_font_rid) const
	{
		return FONT_ANTIALIASING_NONE;
	}

	virtual void font_set_disable_embedded_bitmaps(
		const RID& p_font_rid, bool p_disable_embedded_bitmaps)
	{
	}

	virtual bool font_get_disable_embedded_bitmaps(const RID& p_font_rid) const { return false; }

	virtual void font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps) {}

	virtual bool font_get_generate_mipmaps(const RID& p_font_rid) const { return false; }

	virtual void font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf) {}

	virtual bool font_is_multichannel_signed_distance_field(const RID& p_font_rid) const
	{
		return false;
	}

	virtual void font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range) {}

	virtual int64_t font_get_msdf_pixel_range(const RID& p_font_rid) const { return 0; }

	virtual void font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size) {}

	virtual int64_t font_get_msdf_size(const RID& p_font_rid) const { return 0; }

	virtual void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) {}

	virtual int64_t font_get_fixed_size(const RID& p_font_rid) const { return 0; }

	virtual void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, FixedSizeScaleMode p_fixed_size_scale)
	{
	}

	virtual FixedSizeScaleMode font_get_fixed_size_scale_mode(const RID& p_font_rid) const
	{
		return FIXED_SIZE_SCALE_DISABLE;
	}

	virtual void font_set_allow_system_fallback(const RID& p_font_rid, bool p_allow_system_fallback)
	{
	}

	virtual bool font_is_allow_system_fallback(const RID& p_font_rid) const { return false; }

	virtual void font_clear_system_fallback_cache() {}

	virtual void font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter) {}

	virtual bool font_is_force_autohinter(const RID& p_font_rid) const { return false; }

	virtual void font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate) {}

	virtual bool font_is_modulate_color_glyphs(const RID& p_font_rid) const { return false; }

	virtual int64_t font_get_palette_count(const RID& p_font_rid) const { return 0; }

	virtual String font_get_palette_name(const RID& p_font_rid, int64_t p_index) const
	{
		return String();
	}

	virtual Vector<Color> font_get_palette_colors(const RID& p_font_rid, int64_t p_index) const
	{
		return Vector<Color>();
	}

	virtual void font_set_palette_custom_colors(
		const RID& p_font_rid, const Vector<Color>& p_colors)
	{
	}

	virtual Vector<Color> font_get_palette_custom_colors(const RID& p_font_rid) const
	{
		return Vector<Color>();
	}

	virtual int64_t font_get_used_palette(const RID& p_font_rid) const { return 0; }

	virtual void font_set_used_palette(const RID& p_font_rid, int64_t p_index) {}

	virtual void font_set_hinting(const RID& p_font_rid, Hinting p_hinting) {}

	virtual Hinting font_get_hinting(const RID& p_font_rid) const { return HINTING_NONE; }

	virtual void font_set_subpixel_positioning(
		const RID& p_font_rid, SubpixelPositioning p_subpixel)
	{
	}

	virtual SubpixelPositioning font_get_subpixel_positioning(const RID& p_font_rid) const
	{
		return SUBPIXEL_POSITIONING_DISABLED;
	}

	virtual void font_set_keep_rounding_remainders(
		const RID& p_font_rid, bool p_keep_rounding_remainders)
	{
	}

	virtual bool font_get_keep_rounding_remainders(const RID& p_font_rid) const { return false; }

	virtual void font_set_embolden(const RID& p_font_rid, double p_strength) {}

	virtual double font_get_embolden(const RID& p_font_rid) const { return 0.0; }

	virtual void font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value) {}

	virtual int64_t font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const
	{
		return 0;
	}

	virtual void font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset) {}

	virtual double font_get_baseline_offset(const RID& p_font_rid) const { return 0.0; }

	virtual void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) {}

	virtual Transform2D font_get_transform(const RID& p_font_rid) const { return Transform2D(); }

	virtual void font_set_oversampling(const RID& p_font_rid, double p_oversampling) {}

	virtual double font_get_oversampling(const RID& p_font_rid) const { return 0.0; }

	virtual Vector<Vector2i> font_get_size_cache_list(const RID& p_font_rid) const
	{
		return Vector<Vector2i>();
	}

	virtual void font_clear_size_cache(const RID& p_font_rid) {}

	virtual void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) {}

	virtual double font_get_ascent(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) {}

	virtual double font_get_descent(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position)
	{
	}

	virtual double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const
	{
		return 0.0;
	}

	virtual void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness)
	{
	}

	virtual double font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const
	{
		return 0.0;
	}

	virtual void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) {}

	virtual double font_get_scale(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual int64_t font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const
	{
		return 0;
	}

	virtual void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index)
	{
	}

	virtual void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image)
	{
	}

	virtual Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
	{
		return Ref<Image>();
	}

	virtual void font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const PackedInt32Array& p_offset)
	{
	}

	virtual PackedInt32Array font_get_texture_offsets(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
	{
		return PackedInt32Array();
	}

	virtual PackedInt32Array font_get_glyph_list(
		const RID& p_font_rid, const Vector2i& p_size) const
	{
		return PackedInt32Array();
	}

	virtual void font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_remove_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph)
	{
	}

	virtual Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance)
	{
	}

	virtual Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset)
	{
	}

	virtual Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size)
	{
	}

	virtual Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Rect2();
	}

	virtual void font_set_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect)
	{
	}

	virtual int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return 0;
	}

	virtual void font_set_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx)
	{
	}

	virtual RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return RID();
	}

	virtual Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Size2();
	}

	virtual void font_clear_kerning_map(const RID& p_font_rid, int64_t p_size) {}

	virtual void font_remove_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair)
	{
	}

	virtual void font_set_kerning(const RID& p_font_rid, int64_t p_size,
		const Vector2i& p_glyph_pair, const Vector2& p_kerning)
	{
	}

	virtual Vector2 font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const
	{
		return Vector2();
	}

	virtual int64_t font_get_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const
	{
		return 0;
	}

	virtual int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const
	{
		return 0;
	}

	virtual bool font_has_char(const RID& p_font_rid, int64_t p_char) const { return false; }

	virtual String font_get_supported_chars(const RID& p_font_rid) const { return String(); }

	virtual PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const
	{
		return PackedInt32Array();
	}

	virtual void font_render_range(
		const RID& p_font, const Vector2i& p_size, int64_t p_start, int64_t p_end)
	{
	}

	virtual void font_render_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_index)
	{
	}

	virtual void font_draw_glyph(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const
	{
	}

	virtual void font_draw_glyph_outline(const RID& p_font, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const
	{
	}

	virtual bool font_is_language_supported(const RID& p_font_rid, const String& p_language) const
	{
		return false;
	}

	virtual void font_set_language_support_override(
		const RID& p_font_rid, const String& p_language, bool p_supported)
	{
	}

	virtual bool font_get_language_support_override(const RID& p_font_rid, const String& p_language)
	{
		return false;
	}

	virtual void font_remove_language_support_override(
		const RID& p_font_rid, const String& p_language)
	{
	}

	virtual PackedStringArray font_get_language_support_overrides(const RID& p_font_rid)
	{
		return PackedStringArray();
	}

	virtual bool font_is_script_supported(const RID& p_font_rid, const String& p_script) const
	{
		return false;
	}

	virtual void font_set_script_support_override(
		const RID& p_font_rid, const String& p_script, bool p_supported)
	{
	}

	virtual bool font_get_script_support_override(const RID& p_font_rid, const String& p_script)
	{
		return false;
	}

	virtual void font_remove_script_support_override(const RID& p_font_rid, const String& p_script)
	{
	}

	virtual PackedStringArray font_get_script_support_overrides(const RID& p_font_rid)
	{
		return PackedStringArray();
	}

#ifndef DISABLE_DEPRECATED
	virtual double font_get_global_oversampling() const { return 1.0; }

	virtual void font_set_global_oversampling(double p_oversampling) {}
#endif
	virtual void reference_oversampling_level(double p_oversampling) {}

	virtual void unreference_oversampling_level(double p_oversampling) {}

	virtual Vector2 get_hex_code_box_size(int64_t p_size, int64_t p_index) const;
	virtual void draw_hex_code_box(const RID& p_canvas, int64_t p_size, const Vector2& p_pos,
		int64_t p_index, const Color& p_color) const;

	/* Shaped text buffer interface */

	virtual RID create_shaped_text(
		Direction p_direction = DIRECTION_AUTO, Orientation p_orientation = ORIENTATION_HORIZONTAL)
	{
		return RID();
	}

	virtual void shaped_text_clear(const RID& p_shaped) {}

	virtual RID shaped_text_duplicate(const RID& p_shaped) { return RID(); }

	virtual void shaped_text_set_direction(
		const RID& p_shaped, Direction p_direction = DIRECTION_AUTO)
	{
	}

	virtual Direction shaped_text_get_direction(const RID& p_shaped) const
	{
		return DIRECTION_AUTO;
	}

	virtual Direction shaped_text_get_inferred_direction(const RID& p_shaped) const
	{
		return DIRECTION_AUTO;
	}

	virtual void shaped_text_set_custom_punctuation(const RID& p_shaped, const String& p_punct) {}

	virtual String shaped_text_get_custom_punctuation(const RID& p_shaped) const
	{
		return String();
	}

	virtual void shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char) {}

	virtual int64_t shaped_text_get_custom_ellipsis(const RID& p_shaped) const { return 0; }

	virtual void shaped_text_set_orientation(
		const RID& p_shaped, Orientation p_orientation = ORIENTATION_HORIZONTAL)
	{
	}

	virtual Orientation shaped_text_get_orientation(const RID& p_shaped) const
	{
		return ORIENTATION_HORIZONTAL;
	}

	virtual void shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled) {}

	virtual bool shaped_text_get_preserve_invalid(const RID& p_shaped) const { return false; }

	virtual void shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled) {}

	virtual bool shaped_text_get_preserve_control(const RID& p_shaped) const { return false; }

	virtual void shaped_text_set_spacing(
		const RID& p_shaped, SpacingType p_spacing, int64_t p_value)
	{
	}

	virtual int64_t shaped_text_get_spacing(const RID& p_shaped, SpacingType p_spacing) const
	{
		return 0;
	}

	virtual String shaped_get_text(const RID& p_shaped) const { return String(); }

	virtual int64_t shaped_get_span_count(const RID& p_shaped) const { return 0; }

	virtual String shaped_get_span_text(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual int64_t shaped_get_run_count(const RID& p_shaped) const { return 0; }

	virtual String shaped_get_run_text(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual Vector2i shaped_get_run_range(const RID& p_shaped, int64_t p_index) const
	{
		return Vector2i();
	}

	virtual Vector2i shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const
	{
		return Vector2i();
	}

	virtual RID shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const
	{
		return RID();
	}

	virtual int shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const { return 0; }

	virtual String shaped_get_run_language(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual Direction shaped_get_run_direction(const RID& p_shaped, int64_t p_index) const
	{
		return DIRECTION_AUTO;
	}

	virtual RID shaped_text_substr(const RID& p_shaped, int64_t p_start, int64_t p_length) const
	{
		return RID();
	} // Copy shaped substring (e.g. line break) without reshaping, but correctly reordered,
	  // preservers range.

	virtual RID shaped_text_get_parent(const RID& p_shaped) const { return RID(); }

	virtual double shaped_text_fit_to_width(const RID& p_shaped, double p_width,
		uint32_t p_jst_flags = JUSTIFICATION_WORD_BOUND | JUSTIFICATION_KASHIDA)
	{
		return 0.0;
	}

	virtual double shaped_text_tab_align(const RID& p_shaped, const PackedFloat32Array& p_tab_stops)
	{
		return 0.0;
	}

	virtual bool shaped_text_shape(const RID& p_shaped) { return false; }

	virtual bool shaped_text_update_breaks(const RID& p_shaped) { return false; }

	virtual bool shaped_text_update_justification_ops(const RID& p_shaped) { return false; }

	virtual bool shaped_text_is_ready(const RID& p_shaped) const { return false; }

	bool shaped_text_has_visible_chars(const RID& p_shaped) const;

	virtual const Glyph* shaped_text_get_glyphs(const RID& p_shaped) const { return nullptr; }

	virtual const Glyph* shaped_text_sort_logical(const RID& p_shaped) { return nullptr; }

	virtual int64_t shaped_text_get_glyph_count(const RID& p_shaped) const { return 0; }

	virtual Vector2i shaped_text_get_range(const RID& p_shaped) const { return Vector2i(); }

	virtual PackedInt32Array shaped_text_get_line_breaks_adv(const RID& p_shaped,
		const PackedFloat32Array& p_width, int64_t p_start = 0, bool p_once = true,
		uint32_t p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const;
	virtual PackedInt32Array shaped_text_get_line_breaks(const RID& p_shaped, double p_width,
		int64_t p_start = 0, uint32_t p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const;
	virtual PackedInt32Array shaped_text_get_word_breaks(const RID& p_shaped,
		uint32_t p_grapheme_flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_PUNCTUATION,
		uint32_t p_skip_grapheme_flags = GRAPHEME_IS_VIRTUAL) const;

	virtual PackedInt32Array shaped_text_get_character_breaks(const RID& p_shaped) const
	{
		return PackedInt32Array();
	}

	virtual PackedInt32Array string_get_word_breaks(
		const String& p_string, const String& p_language = "", int64_t p_chars_per_line = 0) const
	{
		return PackedInt32Array();
	}

	virtual String string_to_upper(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual String string_to_lower(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual String string_to_title(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual int64_t shaped_text_get_trim_pos(const RID& p_shaped) const { return -1; }

	virtual int64_t shaped_text_get_ellipsis_pos(const RID& p_shaped) const { return -1; }

	virtual const Glyph* shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const
	{
		return nullptr;
	}

	virtual int64_t shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const { return -1; }

	virtual void shaped_text_overrun_trim_to_width(
		const RID& p_shaped, double p_width, uint32_t p_trim_flags)
	{
	}

	virtual Size2 shaped_text_get_size(const RID& p_shaped) const { return Size2(); }

	virtual double shaped_text_get_ascent(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_descent(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_width(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_underline_position(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_underline_thickness(const RID& p_shaped) const { return 0.0; }

	virtual Direction shaped_text_get_dominant_direction_in_range(
		const RID& p_shaped, int64_t p_start, int64_t p_end) const;

	virtual CaretInfo shaped_text_get_carets(const RID& p_shaped, int64_t p_position) const;

	virtual Vector<Vector2> shaped_text_get_selection(
		const RID& p_shaped, int64_t p_start, int64_t p_end) const;

	virtual int64_t shaped_text_hit_test_grapheme(
		const RID& p_shaped, double p_coords) const; // Return grapheme index.
	virtual int64_t shaped_text_hit_test_position(
		const RID& p_shaped, double p_coords) const; // Return caret/selection position.

	virtual Vector2 shaped_text_get_grapheme_bounds(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_next_grapheme_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_prev_grapheme_pos(const RID& p_shaped, int64_t p_pos) const;

	virtual int64_t shaped_text_next_character_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_prev_character_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_closest_character_pos(const RID& p_shaped, int64_t p_pos) const;

	// The pen position is always placed on the baseline and moving left to right.
	virtual void shaped_text_draw(const RID& p_shaped, const RID& p_canvas, const Vector2& p_pos,
		double p_clip_l = -1.0, double p_clip_r = -1.0, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	virtual void shaped_text_draw_outline(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		int64_t p_outline_size = 1, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;

#ifdef DEBUG_ENABLED
	void debug_print_glyph(int p_idx, const Glyph& p_glyph) const;
	void shaped_text_debug_print(const RID& p_shaped) const;
#endif

#ifndef DISABLE_DEPRECATED
	// Number conversion.
	virtual String format_number(const String& p_string, const String& p_language = "") const;
	virtual String parse_number(const String& p_string, const String& p_language = "") const;
	virtual String percent_sign(const String& p_language = "") const;
#endif // DISABLE_DEPRECATED

	// String functions.
	virtual PackedInt32Array string_get_character_breaks(
		const String& p_string, const String& p_language = "") const;


	virtual int64_t is_confusable(const String& p_string, const PackedStringArray& p_dict) const
	{
		return -1;
	}

	virtual bool spoof_check(const String& p_string) const { return false; }

	virtual String strip_diacritics(const String& p_string) const;
	virtual bool is_valid_identifier(const String& p_string) const;
	virtual bool is_valid_letter(uint64_t p_unicode) const;

	static void set_current_drawn_item_oversampling(double p_vp_oversampling)
	{
		vp_oversampling = p_vp_oversampling;
	}

	static double get_current_drawn_item_oversampling() { return vp_oversampling; }

	virtual void cleanup() {}

	TextServer() = default;
	~TextServer() = default;
};

struct Glyph
{
	int start = -1; // Start offset in the source string.
	int end = -1;	// End offset in the source string.

	uint8_t count = 0;	// Number of glyphs in the grapheme, set in the first glyph only.
	uint8_t repeat = 1; // Draw multiple times in the row.
	uint16_t flags = 0; // Grapheme flags (valid, rtl, virtual), set in the first glyph only.

	float x_off = 0.f; // Offset from the origin of the glyph on baseline.
	float y_off = 0.f;
	float advance =
		0.f; // Advance to the next glyph along baseline(x for horizontal layout, y for vertical).

	RID font_rid;	   // Font resource.
	int font_size = 0; // Font size.
	int32_t index = 0; // Glyph index (font specific) or UTF-32 codepoint (for the invalid glyphs).

	int span_index = -1;

	bool operator==(const Glyph& p_a) const;
	bool operator!=(const Glyph& p_a) const;

	bool operator<(const Glyph& p_a) const;
	bool operator>(const Glyph& p_a) const;
};

struct CaretInfo
{
	Rect2 l_caret;
	Rect2 t_caret;
	TextServer::Direction l_dir;
	TextServer::Direction t_dir;
};

class TextServerManager final
{
private:
	struct Data
	{
		Ref<TextServer> primary_interface;
		Vector<Ref<TextServer>> interfaces;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void add_interface(const Ref<TextServer>& p_interface);
	static void remove_interface(const Ref<TextServer>& p_interface);
	static int get_interface_count();
	static Ref<TextServer> get_interface(int p_index);
	static Ref<TextServer> find_interface(const String& p_name);

	_FORCE_INLINE_ static Ref<TextServer> get_primary_interface()
	{
		return data ? data->primary_interface : Ref<TextServer>();
	}

	static void set_primary_interface(const Ref<TextServer>& p_primary_interface);

	TextServerManager() = delete;
	TextServerManager(const TextServerManager&) = delete;
	TextServerManager& operator=(const TextServerManager&) = delete;
	~TextServerManager() = delete;
};

#define TS TextServerManager::get_primary_interface()

// --- Folded from servers/text/text_server_dummy.h ---
/*************************************************************************/

#include "core/os/os.h"


class TextServerDummy : public TextServer
{
	_THREAD_SAFE_CLASS_

public:
	virtual bool has_feature(Feature p_feature) const override { return false; }

	virtual String get_name() const override { return "Dummy"; }

	virtual String get_short_name() const override { return "dummy"; }

	virtual int64_t get_features() const override { return 0; }

	virtual void free_rid(const RID& p_rid) override {}

	virtual bool has(const RID& p_rid) override { return false; }

	virtual RID create_font() override { return RID(); }

	virtual void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) override {}

	virtual int64_t font_get_fixed_size(const RID& p_font_rid) const override { return 0; }

	virtual void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode) override
	{
	}

	virtual TextServer::FixedSizeScaleMode font_get_fixed_size_scale_mode(
		const RID& p_font_rid) const override
	{
		return FIXED_SIZE_SCALE_DISABLE;
	}

	virtual void font_clear_size_cache(const RID& p_font_rid) override {}

	virtual void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) override {}

	virtual double font_get_ascent(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) override
	{
	}

	virtual double font_get_descent(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position) override
	{
	}

	virtual double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness) override
	{
	}

	virtual double font_get_underline_thickness(
		const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) override {}

	virtual double font_get_scale(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual int64_t font_get_texture_count(
		const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return 0;
	}

	virtual void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) override
	{
	}

	virtual void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image) override
	{
	}

	virtual Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const override
	{
		return Ref<Image>();
	}

	virtual PackedInt32Array font_get_glyph_list(
		const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return PackedInt32Array();
	}

	virtual void font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_remove_glyph(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) override
	{
	}

	virtual Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance) override
	{
	}

	virtual Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_offset(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, const Vector2& p_offset) override
	{
	}

	virtual Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_size(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_gl_size) override
	{
	}

	virtual Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Rect2();
	}

	virtual void font_set_glyph_uv_rect(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, const Rect2& p_uv_rect) override
	{
	}

	virtual int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return 0;
	}

	virtual void font_set_glyph_texture_idx(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, int64_t p_texture_idx) override
	{
	}

	virtual RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return RID();
	}

	virtual Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Size2();
	}

	virtual int64_t font_get_glyph_index(const RID& p_font_rid, int64_t p_size, int64_t p_char,
		int64_t p_variation_selector) const override
	{
		return 0;
	}

	virtual int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const override
	{
		return 0;
	}

	virtual bool font_has_char(const RID& p_font_rid, int64_t p_char) const override
	{
		return false;
	}

	virtual String font_get_supported_chars(const RID& p_font_rid) const override
	{
		return String();
	}

	virtual PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const override
	{
		return PackedInt32Array();
	}

	virtual void font_draw_glyph(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const override
	{
	}

	virtual void font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const override
	{
	}

	virtual RID create_shaped_text(
		TextServer::Direction p_direction, TextServer::Orientation p_orientation) override
	{
		return RID();
	}

	virtual void shaped_text_clear(const RID& p_shaped) override {}

	virtual RID shaped_text_duplicate(const RID& p_shaped) override { return RID(); }

	virtual int64_t shaped_get_span_count(const RID& p_shaped) const override { return 0; }

	virtual RID shaped_text_substr(
		const RID& p_shaped, int64_t p_start, int64_t p_length) const override
	{
		return RID();
	}

	virtual RID shaped_text_get_parent(const RID& p_shaped) const override { return RID(); }

	virtual bool shaped_text_shape(const RID& p_shaped) override { return false; }

	virtual bool shaped_text_is_ready(const RID& p_shaped) const override { return false; }

	virtual const Glyph* shaped_text_get_glyphs(const RID& p_shaped) const override
	{
		return nullptr;
	}

	virtual const Glyph* shaped_text_sort_logical(const RID& p_shaped) override { return nullptr; }

	virtual int64_t shaped_text_get_glyph_count(const RID& p_shaped) const override { return 0; }

	virtual Vector2i shaped_text_get_range(const RID& p_shaped) const override
	{
		return Vector2i();
	}

	virtual int64_t shaped_text_get_trim_pos(const RID& p_shaped) const override { return -1; }

	virtual int64_t shaped_text_get_ellipsis_pos(const RID& p_shaped) const override { return -1; }

	virtual const Glyph* shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const override
	{
		return nullptr;
	}

	virtual int64_t shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const override
	{
		return -1;
	}

	virtual Size2 shaped_text_get_size(const RID& p_shaped) const override { return Size2(); }

	virtual double shaped_text_get_ascent(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_descent(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_width(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_underline_position(const RID& p_shaped) const override
	{
		return 0;
	}

	virtual double shaped_text_get_underline_thickness(const RID& p_shaped) const override
	{
		return 0;
	}

	virtual void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) override
	{
	}
};



// --- End servers/text/text_server_dummy.h ---
// --- End servers/text/text_server.h ---

class TextLine;
class TextParagraph;

class Font : public Resource
{
	struct ShapedTextKey
	{
		String text;
		int font_size = 14;
		float width = 0.f;
		uint32_t jst_flags = TextServer::JUSTIFICATION_NONE;
		uint32_t brk_flags = TextServer::BREAK_MANDATORY;
		TextServer::Direction direction = TextServer::DIRECTION_AUTO;
		TextServer::Orientation orientation = TextServer::ORIENTATION_HORIZONTAL;

		bool operator==(const ShapedTextKey& p_b) const
		{
			return (font_size == p_b.font_size) && (width == p_b.width) &&
				   (jst_flags == p_b.jst_flags) && (brk_flags == p_b.brk_flags) &&
				   (direction == p_b.direction) && (orientation == p_b.orientation) &&
				   (text == p_b.text);
		}

		ShapedTextKey() {}

		ShapedTextKey(const String& p_text, int p_font_size, float p_width, uint32_t p_jst_flags,
			uint32_t p_brk_flags, TextServer::Direction p_direction,
			TextServer::Orientation p_orientation)
		{
			text = p_text;
			font_size = p_font_size;
			width = p_width;
			jst_flags = p_jst_flags;
			brk_flags = p_brk_flags;
			direction = p_direction;
			orientation = p_orientation;
		}
	};

	struct ShapedTextKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const ShapedTextKey& p_a)
		{
			uint32_t hash = p_a.text.hash();
			hash = hash_murmur3_one_32(p_a.font_size, hash);
			hash = hash_murmur3_one_float(p_a.width, hash);
			hash = hash_murmur3_one_32(p_a.brk_flags | (p_a.jst_flags << 6) |
										   (p_a.direction << 12) | (p_a.orientation << 15),
				hash);
			return hash_fmix32(hash);
		}
	};

	// Shaped string cache.
	mutable LRUCache<ShapedTextKey, Ref<TextLine>, ShapedTextKeyHasher> cache;
	mutable LRUCache<ShapedTextKey, Ref<TextParagraph>, ShapedTextKeyHasher> cache_wrap;

protected:
	// Output.
	mutable bool dirty_rids = true;

	// Fallbacks.
	static constexpr int MAX_FALLBACK_DEPTH = 64;

	virtual void reset_state() override;
	virtual void _update_rids_fb(const Font* p_f, int p_depth) const;
	virtual void _update_rids() const;

#ifndef DISABLE_DEPRECATED
	void _draw_string_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_string_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		int p_size = 1, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	real_t _draw_char_bind_compat_104872(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	real_t _draw_char_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		char32_t p_char, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	static void _bind_compatibility_methods();
#endif

public:
	virtual bool _is_cyclic(const Ref<Font>& p_f, int p_depth) const;
	virtual bool _is_base_cyclic(const Ref<Font>& p_f, int p_depth) const;
	virtual void _invalidate_rids();

	static constexpr int DEFAULT_FONT_SIZE = 16;

	virtual RID _get_rid() const { return RID(); }

	// Font metrics.
	virtual real_t get_height(int p_font_size) const;
	virtual real_t get_ascent(int p_font_size) const;
	virtual real_t get_descent(int p_font_size) const;
	virtual real_t get_underline_position(int p_font_size) const;
	virtual real_t get_underline_thickness(int p_font_size) const;

	virtual String get_font_name() const;
	virtual String get_font_style_name() const;
	virtual uint32_t get_font_style() const;
	virtual int get_font_weight() const;
	virtual int get_font_stretch() const;

	virtual int64_t get_palette_count() const;
	virtual String get_palette_name(int64_t p_index) const;
	virtual Vector<Color> get_palette_colors(int64_t p_index) const;

	virtual int get_spacing(TextServer::SpacingType p_spacing) const { return 0; }

	// Drawing string.
	virtual void set_cache_capacity(int p_single_line, int p_multi_line);

	virtual Size2 get_string_size(const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	virtual Size2 get_multiline_string_size(const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;

	virtual void draw_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	virtual void draw_multiline_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	virtual void draw_string_outline(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	virtual void draw_multiline_string_outline(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		int p_size = 1, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	// Drawing char.
	virtual Size2 get_char_size(char32_t p_char, int p_font_size = DEFAULT_FONT_SIZE) const;
	virtual real_t draw_char(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		float p_oversampling = 0.0) const;
	virtual real_t draw_char_outline(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0), float p_oversampling = 0.0) const;

	// Helper functions.
	virtual bool has_char(char32_t p_char) const;
	virtual String get_supported_chars() const;

	virtual bool is_language_supported(const String& p_language) const;
	virtual bool is_script_supported(const String& p_script) const;

	virtual int64_t get_face_count() const;

	Font();
	~Font();
};

class FontFile : public Font
{
	// Font source data.
	const uint8_t* data_ptr = nullptr;
	size_t data_size = 0;
	mutable PackedByteArray data;

	TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	bool mipmaps = false;
	bool disable_embedded_bitmaps = true;
	bool msdf = false;
	int msdf_pixel_range = 16;
	int msdf_size = 48;
	int fixed_size = 0;
	TextServer::FixedSizeScaleMode fixed_size_scale_mode = TextServer::FIXED_SIZE_SCALE_DISABLE;
	bool force_autohinter = false;
	bool modulate_color_glyphs = false;
	bool allow_system_fallback = true;
	TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
	TextServer::SubpixelPositioning subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_AUTO;
	bool keep_rounding_remainders = true;
	double oversampling_override = 0.0;

	HashMap<String, bool> language_support_overrides;
	HashMap<String, bool> script_support_overrides;

#ifndef DISABLE_DEPRECATED
	real_t bmp_height = 0.0;
	real_t bmp_ascent = 0.0;
#endif

	// Cache.
	mutable Vector<RID> cache;

	_FORCE_INLINE_ void _clear_cache();

	void _convert_packed_8bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_packed_4bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_rgba_4bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_mono_8bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol);
	void _convert_mono_4bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol);

protected:
	virtual void reset_state() override;

public:
	Error _load_bitmap_font(const String& p_path, List<String>* r_image_files);

	Error load_bitmap_font(const String& p_path);
	Error load_dynamic_font(const String& p_path);

	// Font source data.
	virtual void set_data_ptr(const uint8_t* p_data, size_t p_size);
	virtual void set_data(const PackedByteArray& p_data);
	virtual PackedByteArray get_data() const;

	// Common properties.
	virtual void set_font_name(const String& p_name);
	virtual void set_font_style_name(const String& p_name);
	virtual void set_font_style(uint32_t p_style);
	virtual void set_font_weight(int p_weight);
	virtual void set_font_stretch(int p_stretch);

	virtual void set_antialiasing(TextServer::FontAntialiasing p_antialiasing);
	virtual TextServer::FontAntialiasing get_antialiasing() const;

	virtual void set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps);
	virtual bool get_disable_embedded_bitmaps() const;

	virtual void set_generate_mipmaps(bool p_generate_mipmaps);
	virtual bool get_generate_mipmaps() const;

	virtual void set_multichannel_signed_distance_field(bool p_msdf);
	virtual bool is_multichannel_signed_distance_field() const;

	virtual void set_msdf_pixel_range(int p_msdf_pixel_range);
	virtual int get_msdf_pixel_range() const;

	virtual void set_msdf_size(int p_msdf_size);
	virtual int get_msdf_size() const;

	virtual void set_fixed_size(int p_fixed_size);
	virtual int get_fixed_size() const;

	virtual void set_fixed_size_scale_mode(TextServer::FixedSizeScaleMode p_fixed_size_scale_mode);
	virtual TextServer::FixedSizeScaleMode get_fixed_size_scale_mode() const;

	virtual void set_allow_system_fallback(bool p_allow_system_fallback);
	virtual bool is_allow_system_fallback() const;

	virtual void set_force_autohinter(bool p_force_autohinter);
	virtual bool is_force_autohinter() const;

	virtual void set_modulate_color_glyphs(bool p_modulate);
	virtual bool is_modulate_color_glyphs() const;

	virtual void set_hinting(TextServer::Hinting p_hinting);
	virtual TextServer::Hinting get_hinting() const;

	virtual void set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel);
	virtual TextServer::SubpixelPositioning get_subpixel_positioning() const;

	virtual void set_keep_rounding_remainders(bool p_keep_rounding_remainders);
	virtual bool get_keep_rounding_remainders() const;

	virtual void set_oversampling(real_t p_oversampling);
	virtual real_t get_oversampling() const;

	virtual RID _get_rid() const override;

	virtual int get_cache_count() const;
	virtual void clear_cache();
	virtual void remove_cache(int p_cache_index);

	virtual void clear_size_cache(int p_cache_index);
	virtual void remove_size_cache(int p_cache_index, const Vector2i& p_size);

	virtual void set_embolden(int p_cache_index, float p_strength);
	virtual float get_embolden(int p_cache_index) const;

	virtual void set_transform(int p_cache_index, Transform2D p_transform);
	virtual Transform2D get_transform(int p_cache_index) const;

	virtual void set_extra_spacing(
		int p_cache_index, TextServer::SpacingType p_spacing, int64_t p_value);
	virtual int64_t get_extra_spacing(int p_cache_index, TextServer::SpacingType p_spacing) const;

	virtual float get_extra_baseline_offset(int p_cache_index) const;
	virtual void set_extra_baseline_offset(int p_cache_index, float p_baseline_offset);

	virtual void set_face_index(int p_cache_index, int64_t p_index);
	virtual int64_t get_face_index(int p_cache_index) const;

	virtual void set_cache_ascent(int p_cache_index, int p_size, real_t p_ascent);
	virtual real_t get_cache_ascent(int p_cache_index, int p_size) const;

	virtual void set_cache_descent(int p_cache_index, int p_size, real_t p_descent);
	virtual real_t get_cache_descent(int p_cache_index, int p_size) const;

	virtual void set_cache_underline_position(
		int p_cache_index, int p_size, real_t p_underline_position);
	virtual real_t get_cache_underline_position(int p_cache_index, int p_size) const;

	virtual void set_cache_underline_thickness(
		int p_cache_index, int p_size, real_t p_underline_thickness);
	virtual real_t get_cache_underline_thickness(int p_cache_index, int p_size) const;

	virtual void set_cache_scale(int p_cache_index, int p_size,
		real_t p_scale); // Rendering scale for bitmap fonts (e.g. emoji fonts).
	virtual real_t get_cache_scale(int p_cache_index, int p_size) const;

	virtual int get_texture_count(int p_cache_index, const Vector2i& p_size) const;
	virtual void clear_textures(int p_cache_index, const Vector2i& p_size);
	virtual void remove_texture(int p_cache_index, const Vector2i& p_size, int p_texture_index);

	virtual void set_texture_image(
		int p_cache_index, const Vector2i& p_size, int p_texture_index, const Ref<Image>& p_image);
	virtual Ref<Image> get_texture_image(
		int p_cache_index, const Vector2i& p_size, int p_texture_index) const;

	virtual void set_texture_offsets(int p_cache_index, const Vector2i& p_size, int p_texture_index,
		const PackedInt32Array& p_offset);
	virtual PackedInt32Array get_texture_offsets(
		int p_cache_index, const Vector2i& p_size, int p_texture_index) const;

	virtual PackedInt32Array get_glyph_list(int p_cache_index, const Vector2i& p_size) const;
	virtual void clear_glyphs(int p_cache_index, const Vector2i& p_size);
	virtual void remove_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_glyph);

	virtual void set_glyph_advance(
		int p_cache_index, int p_size, int32_t p_glyph, const Vector2& p_advance);
	virtual Vector2 get_glyph_advance(int p_cache_index, int p_size, int32_t p_glyph) const;

	virtual void set_glyph_offset(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_offset);
	virtual Vector2 get_glyph_offset(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_size(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_gl_size);
	virtual Vector2 get_glyph_size(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_uv_rect(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Rect2& p_uv_rect);
	virtual Rect2 get_glyph_uv_rect(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_texture_idx(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, int p_texture_idx);
	virtual int get_glyph_texture_idx(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void clear_kerning_map(int p_cache_index, int p_size);
	virtual void remove_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair);

	virtual void set_kerning(
		int p_cache_index, int p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning);
	virtual Vector2 get_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) const;

	virtual void render_range(
		int p_cache_index, const Vector2i& p_size, char32_t p_start, char32_t p_end);
	virtual void render_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_index);

	// Language/script support override.
	virtual void set_language_support_override(const String& p_language, bool p_supported);
	virtual bool get_language_support_override(const String& p_language) const;
	virtual void remove_language_support_override(const String& p_language);
	virtual Vector<String> get_language_support_overrides() const;

	virtual void set_script_support_override(const String& p_script, bool p_supported);
	virtual bool get_script_support_override(const String& p_script) const;
	virtual void remove_script_support_override(const String& p_script);
	virtual Vector<String> get_script_support_overrides() const;

	// Base font properties.
	virtual int32_t get_glyph_index(
		int p_size, char32_t p_char, char32_t p_variation_selector = 0x0000) const;
	virtual char32_t get_char_from_glyph_index(int p_size, int32_t p_glyph_index) const;

	FontFile();
	~FontFile();
};

class FontVariation : public Font
{
	struct Variation
	{
		real_t embolden = 0.f;
		int face_index = 0;
		Transform2D transform;
	};

	mutable Ref<Font> theme_font;

	Ref<Font> base_font;

	Variation variation;
	int extra_spacing[TextServer::SPACING_MAX];
	float baseline_offset = 0.0;
	int64_t palette_index = 0;
	Vector<Color> custom_colors;

protected:
	virtual void _update_rids() const override;

	virtual void reset_state() override;

public:
	virtual Ref<Font> get_base_font() const;
	virtual void set_base_font(const Ref<Font>& p_font);
	virtual Ref<Font> _get_base_font_or_default() const;

	virtual void set_variation_embolden(float p_strength);
	virtual float get_variation_embolden() const;

	virtual void set_variation_transform(Transform2D p_transform);
	virtual Transform2D get_variation_transform() const;

	virtual void set_variation_face_index(int p_face_index);
	virtual int get_variation_face_index() const;

	virtual void set_spacing(TextServer::SpacingType p_spacing, int p_value);
	virtual int get_spacing(TextServer::SpacingType p_spacing) const override;

	virtual float get_baseline_offset() const;
	virtual void set_baseline_offset(float p_baseline_offset);

	virtual int64_t get_palette_index() const;
	virtual void set_palette_index(int64_t p_palette_index);

	virtual Vector<Color> get_palette_custom_colors() const;
	virtual void set_palette_custom_colors(const Vector<Color>& p_colors);

	// Output.
	virtual RID _get_rid() const override;

	FontVariation();
	~FontVariation();
};

class SystemFont : public Font
{
	PackedStringArray names;
	bool italic = false;
	int weight = 400;
	int stretch = 100;

	mutable Ref<Font> theme_font;

	Ref<FontFile> base_font;
	Vector<int> face_indices;
	int ftr_weight = 0;
	int ftr_stretch = 0;
	int ftr_italic = 0;

	TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	bool mipmaps = false;
	bool disable_embedded_bitmaps = true;
	bool force_autohinter = false;
	bool modulate_color_glyphs = false;
	bool allow_system_fallback = true;
	TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
	TextServer::SubpixelPositioning subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_AUTO;
	bool keep_rounding_remainders = true;
	double oversampling_override = 0.0;
	bool msdf = false;
	int msdf_pixel_range = 16;
	int msdf_size = 48;

protected:
	virtual void _update_base_font();
	virtual void _update_rids() const override;

	virtual void reset_state() override;

public:
	virtual Ref<Font> get_base_font() const { return base_font; }

	virtual Ref<Font> _get_base_font_or_default() const;

	virtual void set_antialiasing(TextServer::FontAntialiasing p_antialiasing);
	virtual TextServer::FontAntialiasing get_antialiasing() const;

	virtual void set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps);
	virtual bool get_disable_embedded_bitmaps() const;

	virtual void set_generate_mipmaps(bool p_generate_mipmaps);
	virtual bool get_generate_mipmaps() const;

	virtual void set_allow_system_fallback(bool p_allow_system_fallback);
	virtual bool is_allow_system_fallback() const;

	virtual void set_force_autohinter(bool p_force_autohinter);
	virtual bool is_force_autohinter() const;

	virtual void set_modulate_color_glyphs(bool p_modulate);
	virtual bool is_modulate_color_glyphs() const;

	virtual void set_hinting(TextServer::Hinting p_hinting);
	virtual TextServer::Hinting get_hinting() const;

	virtual void set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel);
	virtual TextServer::SubpixelPositioning get_subpixel_positioning() const;

	virtual void set_keep_rounding_remainders(bool p_keep_rounding_remainders);
	virtual bool get_keep_rounding_remainders() const;

	virtual void set_oversampling(real_t p_oversampling);
	virtual real_t get_oversampling() const;

	virtual void set_multichannel_signed_distance_field(bool p_msdf);
	virtual bool is_multichannel_signed_distance_field() const;

	virtual void set_msdf_pixel_range(int p_msdf_pixel_range);
	virtual int get_msdf_pixel_range() const;

	virtual void set_msdf_size(int p_msdf_size);
	virtual int get_msdf_size() const;

	virtual void set_font_names(const PackedStringArray& p_names);
	virtual PackedStringArray get_font_names() const;

	virtual void set_font_italic(bool p_italic);
	virtual bool get_font_italic() const;

	virtual void set_font_weight(int p_weight);
	virtual int get_font_weight() const override;

	virtual void set_font_stretch(int p_stretch);
	virtual int get_font_stretch() const override;

	virtual int get_spacing(TextServer::SpacingType p_spacing) const override;

	virtual RID _get_rid() const override;

	int64_t get_face_count() const override;

	SystemFont();
	~SystemFont();
};

// --- Folded from scene/resources/text_line.h ---



/*************************************************************************/

class TextLine : public RefCounted
{
private:
	RID rid;

	mutable bool dirty = true;

	float width = -1.0;
	uint32_t flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA;
	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
	String el_char = U"…";
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_TRIM_ELLIPSIS;

	Vector<float> tab_stops;

protected:
#ifndef DISABLE_DEPRECATED
	void _draw_bind_compat_104872(
		RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	static void _bind_compatibility_methods();
#endif


	void _shape() const;

public:
	RID get_rid() const;

	void clear();
	Ref<TextLine> duplicate() const;

	void set_direction(TextServer::Direction p_direction);
	TextServer::Direction get_direction() const;
	TextServer::Direction get_inferred_direction() const;

	void set_orientation(TextServer::Orientation p_orientation);
	TextServer::Orientation get_orientation() const;

	void set_preserve_invalid(bool p_enabled);
	bool get_preserve_invalid() const;

	void set_preserve_control(bool p_enabled);
	bool get_preserve_control() const;

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void tab_align(const Vector<float>& p_tab_stops);

	void set_flags(uint32_t p_flags);
	uint32_t get_flags() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String& p_char);
	String get_ellipsis_char() const;

	void set_width(float p_width);
	float get_width() const;

	Size2 get_size() const;

	float get_line_ascent() const;
	float get_line_descent() const;
	float get_line_width() const;
	float get_line_underline_position() const;
	float get_line_underline_thickness() const;

	void draw(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	void draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	int hit_test(float p_coords) const;

	TextLine(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const String& p_language = "",
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL);
	TextLine();
	~TextLine();
};

// --- Folded from scene/resources/text_paragraph.h ---
#include "core/os/os.h"
#include "core/string/ustring.h"



/*************************************************************************/

class TextParagraph : public RefCounted
{
	_THREAD_SAFE_CLASS_

private:
	mutable RID dropcap_rid;
	mutable int dropcap_lines = 0;
	Rect2 dropcap_margins;

	RID rid;
	mutable LocalVector<RID> lines_rid;

	mutable bool lines_dirty = true;

	float line_spacing = 0.0;
	float width = -1.0;
	int max_lines_visible = -1;

	uint32_t brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND;
	uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
						 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
						 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	String el_char = U"…";
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;

	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;

	Vector<float> tab_stops;

protected:
#ifndef DISABLE_DEPRECATED
	void _draw_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		const Color& p_color = Color(1, 1, 1), const Color& p_dc_color = Color(1, 1, 1)) const;
	void _draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1),
		const Color& p_dc_color = Color(1, 1, 1)) const;
	void _draw_line_bind_compat_104872(RID p_canvas, const Vector2& p_pos, int p_line,
		const Color& p_color = Color(1, 1, 1)) const;
	void _draw_line_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos, int p_line,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_dropcap_bind_compat_104872(
		RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_dropcap_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;

	static void _bind_compatibility_methods();
#endif

	void _shape_lines() const;

public:
	RID get_rid() const;
	RID get_line_rid(int p_line) const;
	RID get_dropcap_rid() const;

	void clear();

	Ref<TextParagraph> duplicate() const;

	void set_direction(TextServer::Direction p_direction);
	TextServer::Direction get_direction() const;
	TextServer::Direction get_inferred_direction() const;

	void set_orientation(TextServer::Orientation p_orientation);
	TextServer::Orientation get_orientation() const;

	void set_preserve_invalid(bool p_enabled);
	bool get_preserve_invalid() const;

	void set_preserve_control(bool p_enabled);
	bool get_preserve_control() const;

	void set_custom_punctuation(const String& p_punct);
	String get_custom_punctuation() const;

	bool set_dropcap(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const Rect2& p_dropcap_margins = Rect2(), const String& p_language = "");
	void clear_dropcap();

	void set_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_alignment() const;

	void tab_align(const Vector<float>& p_tab_stops);

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_break_flags(uint32_t p_flags);
	uint32_t get_break_flags() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String& p_char);
	String get_ellipsis_char() const;

	void set_width(float p_width);
	float get_width() const;

	void set_max_lines_visible(int p_lines);
	int get_max_lines_visible() const;

	void set_line_spacing(float p_spacing);
	float get_line_spacing() const;

	Size2 get_non_wrapped_size() const;

	Size2 get_size() const;

	Vector2i get_range() const;

	int get_line_count() const;

	Size2 get_line_size(int p_line) const;
	float get_line_ascent(int p_line) const;
	float get_line_descent(int p_line) const;
	float get_line_width(int p_line) const;
	Vector2i get_line_range(int p_line) const;
	float get_line_underline_position(int p_line) const;
	float get_line_underline_thickness(int p_line) const;

	Size2 get_dropcap_size() const;
	int get_dropcap_lines() const;

	void draw(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		const Color& p_dc_color = Color(1, 1, 1), float p_oversampling = 0.0) const;
	void draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), const Color& p_dc_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;

	void draw_line(RID p_canvas, const Vector2& p_pos, int p_line,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;
	void draw_line_outline(RID p_canvas, const Vector2& p_pos, int p_line, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	void draw_dropcap(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	void draw_dropcap_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	int hit_test(const Point2& p_coords) const;

	bool is_dirty();

	Mutex& get_mutex() const { return _thread_safe_; }

	TextParagraph(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const String& p_language = "", float p_width = -1.f,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL);
	TextParagraph();
	~TextParagraph();
};
// --- End scene/resources/text_line.h ---
// --- End scene/resources/font.h ---
// --- Folded from scene/resources/style_box.h ---
#include "core/types.h"
#include "servers/display/display_server.h"

class CanvasItem;

class StyleBox : public Resource
{
public:
	enum StyleType
	{
		STYLE_EMPTY,
		STYLE_FLAT,
		STYLE_TEXTURE,
		STYLE_LINE,
	};

	enum AxisStretchMode
	{
		AXIS_STRETCH_MODE_STRETCH,
		AXIS_STRETCH_MODE_TILE,
		AXIS_STRETCH_MODE_TILE_FIT,
	};

private:
	StyleType style_type = STYLE_EMPTY;
	float content_margin[4];

	// Shared by STYLE_FLAT and STYLE_TEXTURE.
	real_t expand_margin[4] = {};
	bool draw_center = true;

	// STYLE_FLAT fields.
	Color bg_color = Color(0.6, 0.6, 0.6);
	Color shadow_color = Color(0, 0, 0, 0.6);
	Color border_color = Color(0.8, 0.8, 0.8);
	real_t border_width[4] = {};
	real_t corner_radius[4] = {};
	bool blend_border = false;
	Vector2 skew;
	bool anti_aliased = true;
	int corner_detail = 8;
	int shadow_size = 0;
	Point2 shadow_offset;
	real_t aa_size = 1;

	// STYLE_TEXTURE fields.
	float texture_margin[4] = {};
	Rect2 region_rect;
	Ref<Texture2D> texture;
	Color modulate = Color(1, 1, 1, 1);
	AxisStretchMode axis_h = AXIS_STRETCH_MODE_STRETCH;
	AxisStretchMode axis_v = AXIS_STRETCH_MODE_STRETCH;

	// STYLE_LINE fields.
	Color color;
	int thickness = 1;
	bool vertical = false;
	float grow_begin = 1.0;
	float grow_end = 1.0;

	void _draw_flat(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_texture(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_line(RID p_canvas_item, const Rect2& p_rect) const;

protected:
	float get_style_margin(Side p_side) const;

public:
	void set_style_type(StyleType p_type);
	StyleType get_style_type() const { return style_type; }

	Size2 get_minimum_size() const;

	void set_content_margin(Side p_side, float p_value);
	void set_content_margin_all(float p_value);
	void set_content_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_content_margin(Side p_side) const;

	float get_margin(Side p_side) const;
	Point2 get_offset() const;

	void draw(RID p_canvas_item, const Rect2& p_rect) const;
	Rect2 get_draw_rect(const Rect2& p_rect) const;

	CanvasItem* get_current_item_drawn() const;

	bool test_mask(const Point2& p_point, const Rect2& p_rect) const;

	// Shared STYLE_FLAT / STYLE_TEXTURE properties.
	void set_expand_margin(Side p_expand_side, float p_size);
	void set_expand_margin_all(float p_expand_margin_size);
	void set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_expand_margin(Side p_expand_side) const;

	void set_draw_center(bool p_enabled);
	bool is_draw_center_enabled() const;

	// STYLE_FLAT properties.
	void set_bg_color(const Color& p_color);
	Color get_bg_color() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_border_width_all(int p_size);
	int get_border_width_min() const;

	void set_border_width(Side p_side, int p_width);
	int get_border_width(Side p_side) const;

	void set_border_blend(bool p_blend);
	bool get_border_blend() const;

	void set_corner_radius_all(int radius);
	void set_corner_radius_individual(const int radius_top_left, const int radius_top_right,
		const int radius_bottom_right, const int radius_bottom_left);
	void set_corner_radius(Corner p_corner, const int radius);
	int get_corner_radius(Corner p_corner) const;

	void set_corner_detail(const int& p_corner_detail);
	int get_corner_detail() const;

	void set_skew(Vector2 p_skew);
	Vector2 get_skew() const;

	void set_shadow_color(const Color& p_color);
	Color get_shadow_color() const;

	void set_shadow_size(const int& p_size);
	int get_shadow_size() const;

	void set_shadow_offset(const Point2& p_offset);
	Point2 get_shadow_offset() const;

	void set_anti_aliased(const bool& p_anti_aliased);
	bool is_anti_aliased() const;
	void set_aa_size(const real_t p_aa_size);
	real_t get_aa_size() const;

	// STYLE_TEXTURE properties.
	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_margin(Side p_side, float p_size);
	void set_texture_margin_all(float p_size);
	void set_texture_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_texture_margin(Side p_side) const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;

	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	void set_modulate(const Color& p_modulate);
	Color get_modulate() const;

	// STYLE_LINE properties.
	void set_color(const Color& p_color);
	Color get_color() const;

	void set_thickness(int p_thickness);
	int get_thickness() const;

	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	void set_grow_begin(float p_grow);
	float get_grow_begin() const;

	void set_grow_end(float p_grow);
	float get_grow_end() const;

	StyleBox(StyleType p_type = STYLE_EMPTY);
};
// --- End scene/resources/style_box.h ---
#include "servers/display/display_server.h"

class Theme : public Resource
{
#ifdef TOOLS_ENABLED
	friend class ThemeItemImportTree;
	friend class ThemeItemEditorDialog;
	friend class ThemeTypeEditor;
#endif

public:
	using ThemeIconMap = HashMap<String, Ref<Texture2D>>;
	using ThemeStyleMap = HashMap<String, Ref<StyleBox>>;
	using ThemeFontMap = HashMap<String, Ref<Font>>;
	using ThemeFontSizeMap = HashMap<String, int>;
	using ThemeColorMap = HashMap<String, Color>;
	using ThemeConstantMap = HashMap<String, int>;

	enum DataType
	{
		DATA_TYPE_COLOR,
		DATA_TYPE_CONSTANT,
		DATA_TYPE_FONT,
		DATA_TYPE_FONT_SIZE,
		DATA_TYPE_ICON,
		DATA_TYPE_STYLEBOX,
		DATA_TYPE_MAX
	};

private:
	bool no_change_propagation = false;

	void _emit_theme_changed(bool p_notify_list_changed = false);

	Vector<String> _get_icon_list(const String& p_theme_type) const;
	Vector<String> _get_icon_type_list() const;
	Vector<String> _get_stylebox_list(const String& p_theme_type) const;
	Vector<String> _get_stylebox_type_list() const;
	Vector<String> _get_font_list(const String& p_theme_type) const;
	Vector<String> _get_font_type_list() const;
	Vector<String> _get_font_size_list(const String& p_theme_type) const;
	Vector<String> _get_font_size_type_list() const;
	Vector<String> _get_color_list(const String& p_theme_type) const;
	Vector<String> _get_color_type_list() const;
	Vector<String> _get_constant_list(const String& p_theme_type) const;
	Vector<String> _get_constant_type_list() const;

	Vector<String> _get_theme_item_list(DataType p_data_type, const String& p_theme_type) const;
	Vector<String> _get_theme_item_type_list(DataType p_data_type) const;

	Vector<String> _get_type_variation_list(const String& p_theme_type) const;
	Vector<String> _get_type_list() const;

protected:
	// Default values configurable for each individual theme.
	float default_base_scale = 0.0;
	Ref<Font> default_font;
	int default_font_size = -1;

	HashMap<String, ThemeIconMap> icon_map;
	HashMap<String, ThemeStyleMap> style_map;
	HashMap<String, ThemeFontMap> font_map;
	HashMap<String, ThemeFontSizeMap> font_size_map;
	HashMap<String, ThemeColorMap> color_map;
	HashMap<String, ThemeConstantMap> constant_map;
	HashMap<String, String> variation_map;
	HashMap<String, List<String>> variation_base_map;


	void _freeze_change_propagation();
	void _unfreeze_and_propagate_changes();

	virtual void reset_state() override;

public:
	static bool is_valid_type_name(const String& p_name);
	static bool is_valid_item_name(const String& p_name);
	static String validate_type_name(const String& p_name);

	void set_default_base_scale(float p_base_scale);
	float get_default_base_scale() const;
	bool has_default_base_scale() const;

	void set_default_font(const Ref<Font>& p_default_font);
	Ref<Font> get_default_font() const;
	bool has_default_font() const;

	void set_default_font_size(int p_font_size);
	int get_default_font_size() const;
	bool has_default_font_size() const;

	void set_icon(
		const String& p_name, const String& p_theme_type, const Ref<Texture2D>& p_icon);
	virtual Ref<Texture2D> get_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_icon(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_icon(const String& p_name, const String& p_theme_type);
	void get_icon_list(const String& p_theme_type, List<String>* p_list) const;
	void add_icon_type(const String& p_theme_type);
	void remove_icon_type(const String& p_theme_type);
	void rename_icon_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_icon_type_list(List<String>* p_list) const;

	virtual Ref<StyleBox> get_stylebox(
		const String& p_name, const String& p_theme_type) const;
	bool has_stylebox(const String& p_name, const String& p_theme_type) const;
	bool has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_stylebox(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_stylebox(const String& p_name, const String& p_theme_type);
	void get_stylebox_list(const String& p_theme_type, List<String>* p_list) const;
	void add_stylebox_type(const String& p_theme_type);
	void remove_stylebox_type(const String& p_theme_type);
	void rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_stylebox_type_list(List<String>* p_list) const;

	void set_font(
		const String& p_name, const String& p_theme_type, const Ref<Font>& p_font);
	virtual Ref<Font> get_font(const String& p_name, const String& p_theme_type) const;
	bool has_font(const String& p_name, const String& p_theme_type) const;
	bool has_font_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font(const String& p_name, const String& p_theme_type);
	void get_font_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_type(const String& p_theme_type);
	void remove_font_type(const String& p_theme_type);
	void rename_font_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_type_list(List<String>* p_list) const;

	void set_font_size(const String& p_name, const String& p_theme_type, int p_font_size);
	virtual int get_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font_size(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font_size(const String& p_name, const String& p_theme_type);
	void get_font_size_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_size_type(const String& p_theme_type);
	void remove_font_size_type(const String& p_theme_type);
	void rename_font_size_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_size_type_list(List<String>* p_list) const;

	void set_color(const String& p_name, const String& p_theme_type, const Color& p_color);
	virtual Color get_color(const String& p_name, const String& p_theme_type) const;
	bool has_color(const String& p_name, const String& p_theme_type) const;
	bool has_color_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_color(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_color(const String& p_name, const String& p_theme_type);
	void get_color_list(const String& p_theme_type, List<String>* p_list) const;
	void add_color_type(const String& p_theme_type);
	void remove_color_type(const String& p_theme_type);
	void rename_color_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_color_type_list(List<String>* p_list) const;

	void set_constant(const String& p_name, const String& p_theme_type, int p_constant);
	virtual int get_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_constant(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_constant(const String& p_name, const String& p_theme_type);
	void get_constant_list(const String& p_theme_type, List<String>* p_list) const;
	void add_constant_type(const String& p_theme_type);
	void remove_constant_type(const String& p_theme_type);
	void rename_constant_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_constant_type_list(List<String>* p_list) const;

	bool has_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	bool has_theme_item_nocheck(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	void rename_theme_item(DataType p_data_type, const String& p_old_name,
		const String& p_name, const String& p_theme_type);
	void clear_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type);
	void get_theme_item_list(
		DataType p_data_type, const String& p_theme_type, List<String>* p_list) const;
	void add_theme_item_type(DataType p_data_type, const String& p_theme_type);
	void remove_theme_item_type(DataType p_data_type,
const String& p_theme_type);
	void rename_theme_item_type(
		DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type);
	void get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const;

	void set_type_variation(const String& p_theme_type, const String& p_base_type);
	bool is_type_variation(const String& p_theme_type, const String& p_base_type) const;
	void clear_type_variation(const String& p_theme_type);
	String get_type_variation_base(const String& p_theme_type) const;
	void get_type_variation_list(const String& p_base_type, List<String>* p_list) const;

	void add_type(const String& p_theme_type);
	void remove_type(const String& p_theme_type);
	void rename_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_type_list(List<String>* p_list) const;
	void get_type_dependencies(const String& p_base_type, const String& p_type_variant,
		Vector<String>& r_result);

	void merge_with(const Ref<Theme>& p_other);
	void clear();

	Theme();
	~Theme();
};

// --- Folded from scene/theme/theme_db.h ---
#include <functional>


class Font;
class Node;
class StyleBox;
class Texture2D;
class ThemeContext;

class ThemeDB final
{
public:
	typedef std::function<void(Node*, const String&, const String&)> ThemeItemSetter;

	struct ThemeItemBind
	{
		Theme::DataType data_type;
		String class_name;
		String item_name;
		String type_name;
		bool external = false;

		ThemeItemSetter setter;

		struct SortByType
		{
			_FORCE_INLINE_ bool operator()(const ThemeItemBind& l, const ThemeItemBind& r) const
			{
				return l.data_type < r.data_type;
			}
		};
	};

	struct Data
	{
		// Global Theme resources used by the default theme context.
		Ref<Theme> default_theme;
		Ref<Theme> project_theme;

		// Universal default values, final fallback for every theme.
		float fallback_base_scale = 1.0f;
		Ref<Font> fallback_font;
		int fallback_font_size = 16;
		Ref<Texture2D> fallback_icon;
		Ref<StyleBox> fallback_stylebox;

		// Global theme contexts used to scope global Theme resources.
		ThemeContext* default_theme_context = nullptr;
		HashMap<Node*, ThemeContext*> theme_contexts;

		HashMap<String, HashMap<String, ThemeItemBind>> theme_item_binds;
		HashMap<String, List<ThemeItemBind>> theme_item_binds_list;
	};

	static inline Data* data = nullptr;

private:
	static void _propagate_theme_context(Node* p_from_node, ThemeContext* p_context);
	static void _init_default_theme_context();
	static void _finalize_theme_contexts();
	static void _sort_theme_items();

public:
	ThemeDB() = delete;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void initialize_theme();
	static void initialize_theme_noproject();
	static void finalize_theme();

	// Global Theme resources.
	static void set_default_theme(const Ref<Theme>& p_default);
	static Ref<Theme> get_default_theme();

	static void set_project_theme(const Ref<Theme>& p_project_default);
	static Ref<Theme> get_project_theme();

	// Universal fallback values.
	static void set_fallback_base_scale(float p_base_scale);
	static float get_fallback_base_scale();

	static void set_fallback_font(const Ref<Font>& p_font);
	static Ref<Font> get_fallback_font();

	static void set_fallback_font_size(int p_font_size);
	static int get_fallback_font_size();

	static void set_fallback_icon(const Ref<Texture2D>& p_icon);
	static Ref<Texture2D> get_fallback_icon();

	static void set_fallback_stylebox(const Ref<StyleBox>& p_stylebox);
	static Ref<StyleBox> get_fallback_stylebox();

	static void get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result);

	// Global theme contexts.
	static ThemeContext* create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes);
	static void destroy_theme_context(Node* p_node);

	static ThemeContext* get_theme_context(Node* p_node);
	static ThemeContext* get_default_theme_context();
	static ThemeContext* get_nearest_theme_context(Node* p_for_node);

	// Theme item binding.
	static void bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter);
	static void bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, const String& p_type_name,
		ThemeItemSetter p_setter);
	static void update_class_instance_items(Node* p_instance);

	static void get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
		bool p_include_inherited = false, Theme::DataType p_filter_type = Theme::DATA_TYPE_MAX);
};

class ThemeContext
{
	friend class ThemeDB;

	Node* node = nullptr;
	ThemeContext* parent = nullptr;

	// Themes are stacked in the order of relevance, for easy iteration.
	// This means that the first theme is the one you should check first,
	// and the last theme is the fallback theme where every lookup ends.
	Vector<Ref<Theme>> themes;

	void _emit_changed();

public:
	void set_themes(Vector<Ref<Theme>>& p_themes);
	const Vector<Ref<Theme>> get_themes() const;
	Ref<Theme> get_fallback_theme() const;
};

// --- Folded from scene/theme/theme_owner.h ---


class Control;
class Node;
class ThemeContext;

class ThemeOwner
{
	Node* holder = nullptr;

	Node* owner_node = nullptr;
	ThemeContext* owner_context = nullptr;

	void _owner_context_changed();
	ThemeContext* _get_active_owner_context() const;

	Node* _get_next_owner_node(Node* p_from_node) const;
	Ref<Theme> _get_owner_node_theme(Node* p_owner_node) const;

public:
	// Theme owner node.
	void set_owner_node(Node* p_node);

	Node* get_owner_node() const { return owner_node; }

	bool has_owner_node() const { return owner_node != nullptr; }

	void set_owner_context(ThemeContext* p_context, bool p_propagate = true);

	// Theme propagation.

	void assign_theme_on_parented(Node* p_for_node);
	void clear_theme_on_unparented(Node* p_for_node);
	void propagate_theme_changed(Node* p_to_node, Node* p_owner_node, bool p_notify, bool p_assign);

	// Theme lookup.

	void get_theme_type_dependencies(
		const Node* p_for_node, const String& p_theme_type, Vector<String>& r_result) const;

	bool has_theme_item_in_types(Theme::DataType p_data_type, const String& p_name,
		const Vector<String>& p_theme_types);

	float get_theme_default_base_scale();
	Ref<Font> get_theme_default_font();
	int get_theme_default_font_size();

	ThemeOwner(Node* p_holder) { holder = p_holder; }
};



// --- End scene/theme/theme_owner.h ---
// --- End scene/theme/theme_db.h ---
