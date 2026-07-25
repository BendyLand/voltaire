/**************************************************************************/
/*  text_server_extension.h                                               */
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

#pragma once

#include "core/object/gdvirtual.gen.h"
#include "core/os/thread_safe.h"
#include "core/variant/native_ptr.h"
#include "core/variant/typed_array.h"
#include "servers/text/text_server.h"

class TextServerExtension : public TextServer {
	VLTRCLASS(TextServerExtension, TextServer);

protected:
	_THREAD_SAFE_CLASS_

	static void _bind_methods();

public:
	virtual bool has_feature(Feature p_feature) const override;
	virtual String get_name() const override;
	virtual String get_short_name() const override;
	virtual int64_t get_features() const override;
	virtual void free_rid(const RID &p_rid) override;
	virtual bool has(const RID &p_rid) override;
	virtual bool load_support_data(const String &p_filename) override;
	virtual String get_support_data_filename() const override;
	virtual String get_support_data_info() const override;
	virtual bool save_support_data(const String &p_filename) const override;
	virtual PackedByteArray get_support_data() const override;
	virtual bool is_locale_using_support_data(const String &p_locale) const override;
	virtual bool is_locale_right_to_left(const String &p_locale) const override;
	virtual int64_t name_to_tag(const String &p_name) const override;
	virtual String tag_to_name(int64_t p_tag) const override;
	/* Font interface */
	virtual RID create_font() override;
	virtual RID create_font_linked_variation(const RID &p_font_rid) override;
	virtual void font_set_data(const RID &p_font_rid, const PackedByteArray &p_data) override;
	virtual void font_set_data_ptr(const RID &p_font_rid, const uint8_t *p_data_ptr, int64_t p_data_size) override;
	virtual void font_set_face_index(const RID &p_font_rid, int64_t p_index) override;
	virtual int64_t font_get_face_index(const RID &p_font_rid) const override;
	virtual int64_t font_get_face_count(const RID &p_font_rid) const override;
	virtual void font_set_style(const RID &p_font_rid, BitField<FontStyle> p_style) override;
	virtual BitField<FontStyle> font_get_style(const RID &p_font_rid) const override;
	virtual void font_set_name(const RID &p_font_rid, const String &p_name) override;
	virtual String font_get_name(const RID &p_font_rid) const override;
	virtual Dictionary font_get_ot_name_strings(const RID &p_font_rid) const override;
	virtual void font_set_style_name(const RID &p_font_rid, const String &p_name) override;
	virtual String font_get_style_name(const RID &p_font_rid) const override;
	virtual void font_set_weight(const RID &p_font_rid, int64_t p_weight) override;
	virtual int64_t font_get_weight(const RID &p_font_rid) const override;
	virtual void font_set_stretch(const RID &p_font_rid, int64_t p_stretch) override;
	virtual int64_t font_get_stretch(const RID &p_font_rid) const override;
	virtual void font_set_antialiasing(const RID &p_font_rid, TextServer::FontAntialiasing p_antialiasing) override;
	virtual TextServer::FontAntialiasing font_get_antialiasing(const RID &p_font_rid) const override;
	virtual void font_set_disable_embedded_bitmaps(const RID &p_font_rid, bool p_disable_embedded_bitmaps) override;
	virtual bool font_get_disable_embedded_bitmaps(const RID &p_font_rid) const override;
	virtual void font_set_generate_mipmaps(const RID &p_font_rid, bool p_generate_mipmaps) override;
	virtual bool font_get_generate_mipmaps(const RID &p_font_rid) const override;
	virtual void font_set_multichannel_signed_distance_field(const RID &p_font_rid, bool p_msdf) override;
	virtual bool font_is_multichannel_signed_distance_field(const RID &p_font_rid) const override;
	virtual void font_set_msdf_pixel_range(const RID &p_font_rid, int64_t p_msdf_pixel_range) override;
	virtual int64_t font_get_msdf_pixel_range(const RID &p_font_rid) const override;
	virtual void font_set_msdf_size(const RID &p_font_rid, int64_t p_msdf_size) override;
	virtual int64_t font_get_msdf_size(const RID &p_font_rid) const override;
	virtual void font_set_fixed_size(const RID &p_font_rid, int64_t p_fixed_size) override;
	virtual int64_t font_get_fixed_size(const RID &p_font_rid) const override;
	virtual void font_set_fixed_size_scale_mode(const RID &p_font_rid, FixedSizeScaleMode p_fixed_size_scale) override;
	virtual FixedSizeScaleMode font_get_fixed_size_scale_mode(const RID &p_font_rid) const override;
	virtual void font_set_subpixel_positioning(const RID &p_font_rid, SubpixelPositioning p_subpixel) override;
	virtual SubpixelPositioning font_get_subpixel_positioning(const RID &p_font_rid) const override;
	virtual void font_set_keep_rounding_remainders(const RID &p_font_rid, bool p_keep_rounding_remainders) override;
	virtual bool font_get_keep_rounding_remainders(const RID &p_font_rid) const override;
	virtual void font_set_embolden(const RID &p_font_rid, double p_strength) override;
	virtual double font_get_embolden(const RID &p_font_rid) const override;
	virtual void font_set_spacing(const RID &p_font_rid, SpacingType p_spacing, int64_t p_value) override;
	virtual int64_t font_get_spacing(const RID &p_font_rid, SpacingType p_spacing) const override;
	virtual void font_set_baseline_offset(const RID &p_font_rid, double p_baseline_offset) override;
	virtual double font_get_baseline_offset(const RID &p_font_rid) const override;
	virtual Transform2D font_get_transform(const RID &p_font_rid) const override;
	virtual void font_set_allow_system_fallback(const RID &p_font_rid, bool p_allow_system_fallback) override;
	virtual bool font_is_allow_system_fallback(const RID &p_font_rid) const override;
	virtual void font_clear_system_fallback_cache() override;
	virtual void font_set_force_autohinter(const RID &p_font_rid, bool p_force_autohinter) override;
	virtual bool font_is_force_autohinter(const RID &p_font_rid) const override;
	virtual void font_set_modulate_color_glyphs(const RID &p_font_rid, bool p_modulate) override;
	virtual bool font_is_modulate_color_glyphs(const RID &p_font_rid) const override;
	virtual int64_t font_get_palette_count(const RID &p_font_rid) const override;
	virtual String font_get_palette_name(const RID &p_font_rid, int64_t p_index) const override;
	virtual Vector<Color> font_get_palette_colors(const RID &p_font_rid, int64_t p_index) const override;
	virtual void font_set_palette_custom_colors(const RID &p_font_rid, const Vector<Color> &p_colors) override;
	virtual Vector<Color> font_get_palette_custom_colors(const RID &p_font_rid) const override;
	virtual int64_t font_get_used_palette(const RID &p_font_rid) const override;
	virtual void font_set_used_palette(const RID &p_font_rid, int64_t p_index) override;
	virtual void font_set_hinting(const RID &p_font_rid, Hinting p_hinting) override;
	virtual Hinting font_get_hinting(const RID &p_font_rid) const override;
	virtual void font_set_variation_coordinates(const RID &p_font_rid, const Dictionary &p_variation_coordinates) override;
	virtual Dictionary font_get_variation_coordinates(const RID &p_font_rid) const override;
	virtual void font_set_oversampling(const RID &p_font_rid, double p_oversampling) override;
	virtual double font_get_oversampling(const RID &p_font_rid) const override;
	virtual TypedArray<Vector2i> font_get_size_cache_list(const RID &p_font_rid) const override;
	virtual void font_clear_size_cache(const RID &p_font_rid) override;
	virtual void font_remove_size_cache(const RID &p_font_rid, const Vector2i &p_size) override;
	virtual TypedArray<Dictionary> font_get_size_cache_info(const RID &p_font_rid) const override;
	virtual void font_set_ascent(const RID &p_font_rid, int64_t p_size, double p_ascent) override;
	virtual double font_get_ascent(const RID &p_font_rid, int64_t p_size) const override;
	virtual void font_set_descent(const RID &p_font_rid, int64_t p_size, double p_descent) override;
	virtual double font_get_descent(const RID &p_font_rid, int64_t p_size) const override;
	virtual void font_set_underline_position(const RID &p_font_rid, int64_t p_size, double p_underline_position) override;
	virtual double font_get_underline_position(const RID &p_font_rid, int64_t p_size) const override;
	virtual void font_set_underline_thickness(const RID &p_font_rid, int64_t p_size, double p_underline_thickness) override;
	virtual double font_get_underline_thickness(const RID &p_font_rid, int64_t p_size) const override;
	virtual void font_set_scale(const RID &p_font_rid, int64_t p_size, double p_scale) override;
	virtual double font_get_scale(const RID &p_font_rid, int64_t p_size) const override;
	virtual int64_t font_get_texture_count(const RID &p_font_rid, const Vector2i &p_size) const override;
	virtual void font_clear_textures(const RID &p_font_rid, const Vector2i &p_size) override;
	virtual void font_remove_texture(const RID &p_font_rid, const Vector2i &p_size, int64_t p_texture_index) override;
	virtual void font_set_texture_image(const RID &p_font_rid, const Vector2i &p_size, int64_t p_texture_index, const Ref<Image> &p_image) override;
	virtual Ref<Image> font_get_texture_image(const RID &p_font_rid, const Vector2i &p_size, int64_t p_texture_index) const override;
	virtual void font_set_texture_offsets(const RID &p_font_rid, const Vector2i &p_size, int64_t p_texture_index, const PackedInt32Array &p_offset) override;
	virtual PackedInt32Array font_get_texture_offsets(const RID &p_font_rid, const Vector2i &p_size, int64_t p_texture_index) const override;
	virtual PackedInt32Array font_get_glyph_list(const RID &p_font_rid, const Vector2i &p_size) const override;
	virtual void font_clear_glyphs(const RID &p_font_rid, const Vector2i &p_size) override;
	virtual void font_remove_glyph(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) override;
	virtual Vector2 font_get_glyph_advance(const RID &p_font_rid, int64_t p_size, int64_t p_glyph) const override;
	virtual void font_set_glyph_advance(const RID &p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2 &p_advance) override;
	virtual Vector2 font_get_glyph_offset(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual void font_set_glyph_offset(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph, const Vector2 &p_offset) override;
	virtual Vector2 font_get_glyph_size(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual void font_set_glyph_size(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph, const Vector2 &p_gl_size) override;
	virtual Rect2 font_get_glyph_uv_rect(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual void font_set_glyph_uv_rect(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph, const Rect2 &p_uv_rect) override;
	virtual int64_t font_get_glyph_texture_idx(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual void font_set_glyph_texture_idx(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph, int64_t p_texture_idx) override;
	virtual RID font_get_glyph_texture_rid(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual Size2 font_get_glyph_texture_size(const RID &p_font_rid, const Vector2i &p_size, int64_t p_glyph) const override;
	virtual Dictionary font_get_glyph_contours(const RID &p_font, int64_t p_size, int64_t p_index) const override;
	virtual TypedArray<Vector2i> font_get_kerning_list(const RID &p_font_rid, int64_t p_size) const override;
	virtual void font_clear_kerning_map(const RID &p_font_rid, int64_t p_size) override;
	virtual void font_remove_kerning(const RID &p_font_rid, int64_t p_size, const Vector2i &p_glyph_pair) override;
	virtual void font_set_kerning(const RID &p_font_rid, int64_t p_size, const Vector2i &p_glyph_pair, const Vector2 &p_kerning) override;
	virtual Vector2 font_get_kerning(const RID &p_font_rid, int64_t p_size, const Vector2i &p_glyph_pair) const override;
	virtual int64_t font_get_glyph_index(const RID &p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector = 0) const override;
	virtual int64_t font_get_char_from_glyph_index(const RID &p_font_rid, int64_t p_size, int64_t p_glyph_index) const override;
	virtual bool font_has_char(const RID &p_font_rid, int64_t p_char) const override;
	virtual String font_get_supported_chars(const RID &p_font_rid) const override;
	virtual PackedInt32Array font_get_supported_glyphs(const RID &p_font_rid) const override;
	virtual void font_render_range(const RID &p_font, const Vector2i &p_size, int64_t p_start, int64_t p_end) override;
	virtual void font_render_glyph(const RID &p_font_rid, const Vector2i &p_size, int64_t p_index) override;
	virtual void font_draw_glyph(const RID &p_font, const RID &p_canvas, int64_t p_size, const Vector2 &p_pos, int64_t p_index, const Color &p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override;
	virtual void font_draw_glyph_outline(const RID &p_font, const RID &p_canvas, int64_t p_size, int64_t p_outline_size, const Vector2 &p_pos, int64_t p_index, const Color &p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override;
	virtual bool font_is_language_supported(const RID &p_font_rid, const String &p_language) const override;
	virtual void font_set_language_support_override(const RID &p_font_rid, const String &p_language, bool p_supported) override;
	virtual bool font_get_language_support_override(const RID &p_font_rid, const String &p_language) override;
	virtual void font_remove_language_support_override(const RID &p_font_rid, const String &p_language) override;
	virtual PackedStringArray font_get_language_support_overrides(const RID &p_font_rid) override;
	virtual bool font_is_script_supported(const RID &p_font_rid, const String &p_script) const override;
	virtual void font_set_script_support_override(const RID &p_font_rid, const String &p_script, bool p_supported) override;
	virtual bool font_get_script_support_override(const RID &p_font_rid, const String &p_script) override;
	virtual void font_remove_script_support_override(const RID &p_font_rid, const String &p_script) override;
	virtual PackedStringArray font_get_script_support_overrides(const RID &p_font_rid) override;
	virtual void font_set_opentype_feature_overrides(const RID &p_font_rid, const Dictionary &p_overrides) override;
	virtual Dictionary font_get_opentype_feature_overrides(const RID &p_font_rid) const override;
	virtual Dictionary font_supported_feature_list(const RID &p_font_rid) const override;
	virtual Dictionary font_supported_variation_list(const RID &p_font_rid) const override;
#ifndef DISABLE_DEPRECATED
	virtual double font_get_global_oversampling() const override;
	virtual void font_set_global_oversampling(double p_oversampling) override;
#endif
	virtual void reference_oversampling_level(double p_oversampling) override;
	virtual void unreference_oversampling_level(double p_oversampling) override;
	virtual Vector2 get_hex_code_box_size(int64_t p_size, int64_t p_index) const override;
	virtual void draw_hex_code_box(const RID &p_canvas, int64_t p_size, const Vector2 &p_pos, int64_t p_index, const Color &p_color) const override;
	/* Shaped text buffer interface */
	virtual RID create_shaped_text(Direction p_direction = DIRECTION_AUTO, Orientation p_orientation = ORIENTATION_HORIZONTAL) override;
	virtual void shaped_text_clear(const RID &p_shaped) override;
	virtual RID shaped_text_duplicate(const RID &p_shaped) override;
	virtual void shaped_text_set_direction(const RID &p_shaped, Direction p_direction = DIRECTION_AUTO) override;
	virtual Direction shaped_text_get_direction(const RID &p_shaped) const override;
	virtual Direction shaped_text_get_inferred_direction(const RID &p_shaped) const override;
	virtual void shaped_text_set_bidi_override(const RID &p_shaped, const Array &p_override) override;
	virtual void shaped_text_set_custom_punctuation(const RID &p_shaped, const String &p_punct) override;
	virtual String shaped_text_get_custom_punctuation(const RID &p_shaped) const override;
	virtual void shaped_text_set_custom_ellipsis(const RID &p_shaped, int64_t p_char) override;
	virtual int64_t shaped_text_get_custom_ellipsis(const RID &p_shaped) const override;
	virtual void shaped_text_set_orientation(const RID &p_shaped, Orientation p_orientation = ORIENTATION_HORIZONTAL) override;
	virtual Orientation shaped_text_get_orientation(const RID &p_shaped) const override;
	virtual void shaped_text_set_preserve_invalid(const RID &p_shaped, bool p_enabled) override;
	virtual bool shaped_text_get_preserve_invalid(const RID &p_shaped) const override;
	virtual void shaped_text_set_preserve_control(const RID &p_shaped, bool p_enabled) override;
	virtual bool shaped_text_get_preserve_control(const RID &p_shaped) const override;
	virtual void shaped_text_set_spacing(const RID &p_shaped, SpacingType p_spacing, int64_t p_value) override;
	virtual int64_t shaped_text_get_spacing(const RID &p_shaped, SpacingType p_spacing) const override;
	virtual bool shaped_text_add_string(const RID &p_shaped, const String &p_text, const TypedArray<RID> &p_fonts, int64_t p_size, const Dictionary &p_opentype_features = Dictionary(), const String &p_language = "", const Variant &p_meta = Variant()) override;
	virtual bool shaped_text_add_object(const RID &p_shaped, const Variant &p_key, const Size2 &p_size, InlineAlignment p_inline_align = INLINE_ALIGNMENT_CENTER, int64_t p_length = 1, double p_baseline = 0.0) override;
	virtual bool shaped_text_resize_object(const RID &p_shaped, const Variant &p_key, const Size2 &p_size, InlineAlignment p_inline_align = INLINE_ALIGNMENT_CENTER, double p_baseline = 0.0) override;
	virtual bool shaped_text_has_object(const RID &p_shaped, const Variant &p_key) const override;
	virtual String shaped_get_text(const RID &p_shaped) const override;
	virtual int64_t shaped_get_span_count(const RID &p_shaped) const override;
	virtual Variant shaped_get_span_meta(const RID &p_shaped, int64_t p_index) const override;
	virtual Variant shaped_get_span_embedded_object(const RID &p_shaped, int64_t p_index) const override;
	virtual String shaped_get_span_text(const RID &p_shaped, int64_t p_index) const override;
	virtual Variant shaped_get_span_object(const RID &p_shaped, int64_t p_index) const override;
	virtual void shaped_set_span_update_font(const RID &p_shaped, int64_t p_index, const TypedArray<RID> &p_fonts, int64_t p_size, const Dictionary &p_opentype_features = Dictionary()) override;
	virtual int64_t shaped_get_run_count(const RID &p_shaped) const override;
	virtual String shaped_get_run_text(const RID &p_shaped, int64_t p_index) const override;
	virtual Vector2i shaped_get_run_range(const RID &p_shaped, int64_t p_index) const override;
	virtual Vector2i shaped_get_run_glyph_range(const RID &p_shaped, int64_t p_index) const override;
	virtual RID shaped_get_run_font_rid(const RID &p_shaped, int64_t p_index) const override;
	virtual int shaped_get_run_font_size(const RID &p_shaped, int64_t p_index) const override;
	virtual String shaped_get_run_language(const RID &p_shaped, int64_t p_index) const override;
	virtual Direction shaped_get_run_direction(const RID &p_shaped, int64_t p_index) const override;
	virtual Variant shaped_get_run_object(const RID &p_shaped, int64_t p_index) const override;
	virtual RID shaped_text_substr(const RID &p_shaped, int64_t p_start, int64_t p_length) const override;
	virtual RID shaped_text_get_parent(const RID &p_shaped) const override;
	virtual double shaped_text_fit_to_width(const RID &p_shaped, double p_width, BitField<TextServer::JustificationFlag> p_jst_flags = JUSTIFICATION_WORD_BOUND | JUSTIFICATION_KASHIDA) override;
	virtual double shaped_text_tab_align(const RID &p_shaped, const PackedFloat32Array &p_tab_stops) override;
	virtual bool shaped_text_shape(const RID &p_shaped) override;
	virtual bool shaped_text_update_breaks(const RID &p_shaped) override;
	virtual bool shaped_text_update_justification_ops(const RID &p_shaped) override;
	virtual bool shaped_text_is_ready(const RID &p_shaped) const override;
	virtual const Glyph *shaped_text_get_glyphs(const RID &p_shaped) const override;
	virtual const Glyph *shaped_text_sort_logical(const RID &p_shaped) override;
	virtual int64_t shaped_text_get_glyph_count(const RID &p_shaped) const override;
	virtual Vector2i shaped_text_get_range(const RID &p_shaped) const override;
	virtual PackedInt32Array shaped_text_get_line_breaks_adv(const RID &p_shaped, const PackedFloat32Array &p_width, int64_t p_start = 0, bool p_once = true, BitField<TextServer::LineBreakFlag> p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const override;
	virtual PackedInt32Array shaped_text_get_line_breaks(const RID &p_shaped, double p_width, int64_t p_start = 0, BitField<TextServer::LineBreakFlag> p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const override;
	virtual PackedInt32Array shaped_text_get_word_breaks(const RID &p_shaped, BitField<TextServer::GraphemeFlag> p_grapheme_flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_PUNCTUATION, BitField<TextServer::GraphemeFlag> p_skip_grapheme_flags = GRAPHEME_IS_VIRTUAL) const override;
	virtual int64_t shaped_text_get_trim_pos(const RID &p_shaped) const override;
	virtual int64_t shaped_text_get_ellipsis_pos(const RID &p_shaped) const override;
	virtual const Glyph *shaped_text_get_ellipsis_glyphs(const RID &p_shaped) const override;
	virtual int64_t shaped_text_get_ellipsis_glyph_count(const RID &p_shaped) const override;
	virtual void shaped_text_overrun_trim_to_width(const RID &p_shaped, double p_width, BitField<TextServer::TextOverrunFlag> p_trim_flags) override;
	virtual Array shaped_text_get_objects(const RID &p_shaped) const override;
	virtual Rect2 shaped_text_get_object_rect(const RID &p_shaped, const Variant &p_key) const override;
	virtual Vector2i shaped_text_get_object_range(const RID &p_shaped, const Variant &p_key) const override;
	virtual int64_t shaped_text_get_object_glyph(const RID &p_shaped, const Variant &p_key) const override;
	virtual Size2 shaped_text_get_size(const RID &p_shaped) const override;
	virtual double shaped_text_get_ascent(const RID &p_shaped) const override;
	virtual double shaped_text_get_descent(const RID &p_shaped) const override;
	virtual double shaped_text_get_width(const RID &p_shaped) const override;
	virtual double shaped_text_get_underline_position(const RID &p_shaped) const override;
	virtual double shaped_text_get_underline_thickness(const RID &p_shaped) const override;
	virtual Direction shaped_text_get_dominant_direction_in_range(const RID &p_shaped, int64_t p_start, int64_t p_end) const override;
	virtual CaretInfo shaped_text_get_carets(const RID &p_shaped, int64_t p_position) const override;
	virtual Vector<Vector2> shaped_text_get_selection(const RID &p_shaped, int64_t p_start, int64_t p_end) const override;
	virtual int64_t shaped_text_hit_test_grapheme(const RID &p_shaped, double p_coords) const override;
	virtual int64_t shaped_text_hit_test_position(const RID &p_shaped, double p_coords) const override;
	virtual void shaped_text_draw(const RID &p_shaped, const RID &p_canvas, const Vector2 &p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0, const Color &p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override;
	virtual void shaped_text_draw_outline(const RID &p_shaped, const RID &p_canvas, const Vector2 &p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0, int64_t p_outline_size = 1, const Color &p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override;
	virtual Vector2 shaped_text_get_grapheme_bounds(const RID &p_shaped, int64_t p_pos) const override;
	virtual int64_t shaped_text_next_grapheme_pos(const RID &p_shaped, int64_t p_pos) const override;
	virtual int64_t shaped_text_prev_grapheme_pos(const RID &p_shaped, int64_t p_pos) const override;
	virtual PackedInt32Array shaped_text_get_character_breaks(const RID &p_shaped) const override;
	virtual int64_t shaped_text_next_character_pos(const RID &p_shaped, int64_t p_pos) const override;
	virtual int64_t shaped_text_prev_character_pos(const RID &p_shaped, int64_t p_pos) const override;
	virtual int64_t shaped_text_closest_character_pos(const RID &p_shaped, int64_t p_pos) const override;
#ifndef DISABLE_DEPRECATED
	virtual String format_number(const String &p_string, const String &p_language = "") const override;
	virtual String parse_number(const String &p_string, const String &p_language = "") const override;
	virtual String percent_sign(const String &p_language = "") const override;
#endif // DISABLE_DEPRECATED
	virtual String strip_diacritics(const String &p_string) const override;
	virtual PackedInt32Array string_get_word_breaks(const String &p_string, const String &p_language = "", int64_t p_chars_per_line = 0) const override;
	virtual PackedInt32Array string_get_character_breaks(const String &p_string, const String &p_language = "") const override;
	virtual bool is_valid_identifier(const String &p_string) const override;
	virtual bool is_valid_letter(uint64_t p_unicode) const override;
	virtual String string_to_upper(const String &p_string, const String &p_language = "") const override;
	virtual String string_to_lower(const String &p_string, const String &p_language = "") const override;
	virtual String string_to_title(const String &p_string, const String &p_language = "") const override;
	TypedArray<Vector3i> parse_structured_text(StructuredTextParser p_parser_type, const Array &p_args, const String &p_text) const;
	virtual int64_t is_confusable(const String &p_string, const PackedStringArray &p_dict) const override;
	virtual bool spoof_check(const String &p_string) const override;
	virtual void cleanup() override;

	TextServerExtension();
	~TextServerExtension();
};
