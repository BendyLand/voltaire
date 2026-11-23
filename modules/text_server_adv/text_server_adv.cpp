/**************************************************************************/
/*  text_server_adv.cpp                                                   */
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

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "modules/register_module_types.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "modules/modules_enabled.gen.h" // For freetype, msdfgen, svg.
#include "scene/resources/image_texture.h"
// --- Inlined from modules/text_server_adv/text_server_adv.h ---
/*************************************************************************/
/* ICU/HarfBuzz/Graphite backed Text Server implementation with BiDi,    */
/* shaping and advanced font features support.                           */
/*************************************************************************/

#include "core/os/thread_safe.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/safe_refcount.h"
#include "modules/modules_enabled.gen.h" // For freetype, msdfgen, svg.
#include "scene/resources/image_texture.h"
#include "servers/text/text_server.h"

// Thirdparty headers.

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wshadow")
#if defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && __clang_major__ >= 21)
GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunnecessary-virtual-specifier")
#endif

#ifdef MODULE_MSDFGEN_ENABLED
#include "thirdparty/msdfgen/core/edge-selectors.h"
#include "thirdparty/msdfgen/msdfgen.h"
#endif

#include <unicode/ubidi.h>
#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/uiter.h>
#include <unicode/uloc.h>
#include <unicode/unorm2.h>
#include <unicode/uscript.h>
#include <unicode/uspoof.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

VLTR_GCC_WARNING_POP
#if defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && __clang_major__ >= 21)
GODOT_CLANG_WARNING_POP
#endif

#ifdef MODULE_FREETYPE_ENABLED
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_STROKER_H
#include FT_ADVANCES_H
#include FT_MULTIPLE_MASTERS_H
#include FT_BBOX_H
#include FT_SIZES_H
#include FT_MODULE_H
#include FT_CONFIG_OPTIONS_H
#if !defined(FT_CONFIG_OPTION_USE_BROTLI) && !defined(_MSC_VER)
#warning FreeType is configured without Brotli support, built-in fonts will not be available.
#endif
#include <hb-ft.h>
#include <hb-ot.h>
#endif

#include <hb-icu.h>
#include <hb.h>
#if HB_VERSION_ATLEAST(13, 0, 0)
#include <hb-raster.h>
#endif

class ScriptIterator final {
	static const int PAREN_STACK_DEPTH = 128;
	static const int EMOJI_STACK_DEPTH = 32;

public:
	struct ScriptRange {
		int start = 0;
		int end = 0;
		hb_script_t script = HB_SCRIPT_COMMON;
	};
	Vector<ScriptRange> script_ranges;

private:
	inline static bool same_script(int32_t p_script_one, int32_t p_script_two);
	inline static bool is_emoji(UChar32 p_c, UChar32 p_next);

public:
	ScriptIterator(const String &p_string, int p_start, int p_length);
};

class TextServerAdvanced final : public TextServer
{
	_THREAD_SAFE_CLASS_

	struct FeatureInfo
	{
		String name;
		bool hidden = false;
	};

	HashMap<String, int32_t> feature_sets;
	HashMap<int32_t, FeatureInfo> feature_sets_inv;

	enum LineBreakStrictness
	{
		LB_AUTO,
		LB_LOOSE,
		LB_NORMAL,
		LB_STRICT,
	};

	SafeNumeric<TextServer::FontLCDSubpixelLayout> lcd_subpixel_layout{
		TextServer::FontLCDSubpixelLayout::FONT_LCD_SUBPIXEL_LAYOUT_NONE};
	LineBreakStrictness lb_strictness = LB_AUTO;
	void _update_settings();

	void _insert_feature_sets();

	// ICU support data.

	static bool icu_data_loaded;
	static PackedByteArray icu_data;
	mutable USet* allowed = nullptr;
	mutable USpoofChecker* sc_spoof = nullptr;
	mutable USpoofChecker* sc_conf = nullptr;

	mutable HashMap<String, UBreakIterator*> line_break_iterators_per_language;

	UBreakIterator* _create_line_break_iterator_for_locale(
		const String& p_language, UErrorCode* r_err) const;

	// Font cache data.

#ifdef MODULE_FREETYPE_ENABLED
	mutable FT_Library ft_library = nullptr;
#endif

	const int rect_range = 1;

	struct FontTexturePosition
	{
		int32_t index = -1;
		int32_t x = 0;
		int32_t y = 0;

		FontTexturePosition() {}

		FontTexturePosition(int32_t p_id, int32_t p_x, int32_t p_y) : index(p_id), x(p_x), y(p_y) {}
	};

	struct Shelf
	{
		int32_t x = 0;
		int32_t y = 0;
		int32_t w = 0;
		int32_t h = 0;

		FontTexturePosition alloc_shelf(int32_t p_id, int32_t p_w, int32_t p_h)
		{
			if (p_w > w || p_h > h) {
				return FontTexturePosition(-1, 0, 0);
			}
			int32_t xx = x;
			x += p_w;
			w -= p_w;
			return FontTexturePosition(p_id, xx, y);
		}

		Shelf() {}

		Shelf(int32_t p_x, int32_t p_y, int32_t p_w, int32_t p_h) : x(p_x), y(p_y), w(p_w), h(p_h)
		{
		}
	};

	struct ShelfPackTexture
	{
		int32_t texture_w = 1024;
		int32_t texture_h = 1024;

		Ref<Image> image;
		Ref<ImageTexture> texture;
		bool dirty = true;

		List<Shelf> shelves;

		FontTexturePosition pack_rect(int32_t p_id, int32_t p_h, int32_t p_w)
		{
			int32_t y = 0;
			int32_t waste = 0;
			Shelf* best_shelf = nullptr;
			int32_t best_waste = std::numeric_limits<std::int32_t>::max();

			for (Shelf& E : shelves) {
				y += E.h;
				if (p_w > E.w) {
					continue;
				}
				if (p_h == E.h) {
					return E.alloc_shelf(p_id, p_w, p_h);
				}
				if (p_h > E.h) {
					continue;
				}
				if (p_h < E.h) {
					waste = (E.h - p_h) * p_w;
					if (waste < best_waste) {
						best_waste = waste;
						best_shelf = &E;
					}
				}
			}
			if (best_shelf) {
				return best_shelf->alloc_shelf(p_id, p_w, p_h);
			}
			if (p_h <= (texture_h - y) && p_w <= texture_w) {
				List<Shelf>::Element* E = shelves.push_back(Shelf(0, y, texture_w, p_h));
				return E->get().alloc_shelf(p_id, p_w, p_h);
			}
			return FontTexturePosition(-1, 0, 0);
		}

		ShelfPackTexture() {}

		ShelfPackTexture(int32_t p_w, int32_t p_h) : texture_w(p_w), texture_h(p_h) {}
	};

	struct FontGlyph
	{
		bool found = false;
		int texture_idx = -1;
		Rect2 rect;
		Rect2 uv_rect;
		Vector2 advance;
		bool fix_edge = false;
	};

	struct FontAdvanced;

	struct FontForSizeAdvanced
	{
		double ascent = 0.0;
		double descent = 0.0;
		double underline_position = 0.0;
		double underline_thickness = 0.0;
		double scale = 1.0;

		FontAdvanced* owner = nullptr;
		uint32_t viewport_oversampling = 0;

		Vector2i size;

		Vector<ShelfPackTexture> textures;
		HashMap<int64_t, int64_t> inv_glyph_map;
		HashMap<int32_t, FontGlyph> glyph_map;
		HashMap<Vector2i, Vector2> kerning_map;
		hb_font_t* hb_handle = nullptr;
#if HB_VERSION_ATLEAST(13, 0, 0)
		hb_face_t* hb_face = nullptr;
		bool color_paint = false;
#endif

#ifdef MODULE_FREETYPE_ENABLED
		FT_Size fsize = nullptr;
#endif

		~FontForSizeAdvanced()
		{
			if (hb_handle != nullptr) {
				hb_font_destroy(hb_handle);
			}
#ifdef MODULE_FREETYPE_ENABLED
			if (fsize != nullptr) {
				FT_Done_Size(fsize);
			}
#endif
		}
	};

	struct OversamplingLevel
	{
		HashSet<FontForSizeAdvanced*> fonts;
		int32_t refcount = 1;
	};

	mutable HashMap<uint32_t, OversamplingLevel> oversampling_levels;

	struct FontAdvancedLinkedVariation
	{
		RID base_font;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;
	};

	struct FontAdvanced
	{
		Mutex mutex;

		TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
		bool disable_embedded_bitmaps = true;
		bool mipmaps = false;
		bool msdf = false;
		int msdf_range = 14;
		FixedSizeScaleMode fixed_size_scale_mode = FIXED_SIZE_SCALE_DISABLE;
		int msdf_source_size = 48;
		int fixed_size = 0;
		bool allow_system_fallback = true;
		bool force_autohinter = false;
		bool modulate_color_glyphs = false;
		TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
		TextServer::SubpixelPositioning subpixel_positioning =
			TextServer::SUBPIXEL_POSITIONING_AUTO;
		bool keep_rounding_remainders = true;
		double oversampling_override = 0.0;
		double embolden = 0.0;
		Transform2D transform;

		uint32_t style_flags = 0;
		String font_name;
		String style_name;
		int weight = 400;
		int stretch = 100;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;

		HashMap<Vector2i, FontForSizeAdvanced*> cache;

#if HB_VERSION_ATLEAST(13, 0, 0)
		Vector<String> palette_names;
		Vector<PackedColorArray> palette_colors;
		PackedColorArray palette_custom_colors;
		Vector<hb_color_t> palette_custom_colors_hb;
		unsigned int palette_index = 0;

		hb_raster_paint_t* hb_rdr = nullptr;
		hb_raster_draw_t* hb_mono = nullptr;
#endif

		bool face_init = false;
		HashSet<uint32_t> supported_scripts;

		// Language/script support override.
		HashMap<String, bool> language_support_overrides;
		HashMap<String, bool> script_support_overrides;

		PackedByteArray data;
		const uint8_t* data_ptr = nullptr;
		size_t data_size;
		int face_index = 0;

#ifdef MODULE_FREETYPE_ENABLED
		FT_Face face = nullptr;
		FT_StreamRec stream;
#endif

		~FontAdvanced()
		{
			for (const KeyValue<Vector2i, FontForSizeAdvanced*>& E : cache) {
				memdelete(E.value);
			}
			cache.clear();
#if HB_VERSION_ATLEAST(13, 0, 0)
			if (hb_rdr != nullptr) {
				hb_raster_paint_destroy(hb_rdr);
			}
			if (hb_mono != nullptr) {
				hb_raster_draw_destroy(hb_mono);
			}
#endif
#ifdef MODULE_FREETYPE_ENABLED
			if (face != nullptr) {
				FT_Done_Face(face);
			}
#endif
		}
	};

	_FORCE_INLINE_ FontTexturePosition find_texture_pos_for_glyph(FontForSizeAdvanced* p_data,
		int p_color_size, Image::Format p_image_format, int p_width, int p_height,
		bool p_msdf) const;
#ifdef MODULE_FREETYPE_ENABLED
	_FORCE_INLINE_ FontGlyph rasterize_bitmap(FontForSizeAdvanced* p_data, int p_rect_margin,
		FT_Bitmap p_bitmap, int p_yofs, int p_xofs, const Vector2& p_advance, bool p_bgra) const;
#if HB_VERSION_ATLEAST(13, 0, 0)
	_FORCE_INLINE_ FontGlyph rasterize_hb_bitmap(FontForSizeAdvanced* p_data, int p_rect_margin,
		hb_raster_image_t* p_image, const hb_raster_extents_t& p_ext, const Vector2& p_advance,
		bool p_bgra) const;
#endif
#endif
	bool _ensure_glyph(FontAdvanced* p_font_data, const Vector2i& p_size, int32_t p_glyph,
		FontGlyph& r_glyph, uint32_t p_oversampling = 0) const;
	bool _ensure_cache_for_size(FontAdvanced* p_font_data, const Vector2i& p_size,
		FontForSizeAdvanced*& r_cache_for_size, bool p_silent = false,
		uint32_t p_oversampling = 0) const;
	_FORCE_INLINE_ bool _font_validate(const RID& p_font_rid) const;
	static void _generateMTSDF_threaded(void* p_td, uint32_t p_y);

	_FORCE_INLINE_ Vector2i _get_size(const FontAdvanced* p_font_data, int p_size) const
	{
		if (p_font_data->msdf) {
			return Vector2i(p_font_data->msdf_source_size * 64, 0);
		}
		else if (p_font_data->fixed_size > 0) {
			return Vector2i(p_font_data->fixed_size * 64, 0);
		}
		else {
			return Vector2i(p_size * 64, 0);
		}
	}

	_FORCE_INLINE_ Vector2i _get_size_outline(
		const FontAdvanced* p_font_data, const Vector2i& p_size) const
	{
		if (p_font_data->msdf) {
			return Vector2i(p_font_data->msdf_source_size * 64, 0);
		}
		else if (p_font_data->fixed_size > 0) {
			return Vector2i(p_font_data->fixed_size * 64, MIN(p_size.y, 1));
		}
		else {
			return Vector2i(p_size.x * 64, p_size.y);
		}
	}

	_FORCE_INLINE_ double _get_extra_advance(RID p_font_rid, int p_font_size) const;
	_FORCE_INLINE_ bool _get_tag_hidden(int64_t p_tag) const;

	_FORCE_INLINE_ int _font_get_weight_by_name(const String& p_sty_name) const
	{
		String sty_name = p_sty_name.remove_chars(" -");
		if (sty_name.contains("thin") || sty_name.contains("hairline")) {
			return 100;
		}
		else if (sty_name.contains("extralight") || sty_name.contains("ultralight")) {
			return 200;
		}
		else if (sty_name.contains("light")) {
			return 300;
		}
		else if (sty_name.contains("semilight")) {
			return 350;
		}
		else if (sty_name.contains("regular")) {
			return 400;
		}
		else if (sty_name.contains("medium")) {
			return 500;
		}
		else if (sty_name.contains("semibold") || sty_name.contains("demibold")) {
			return 600;
		}
		else if (sty_name.contains("bold")) {
			return 700;
		}
		else if (sty_name.contains("extrabold") || sty_name.contains("ultrabold")) {
			return 800;
		}
		else if (sty_name.contains("black") || sty_name.contains("heavy")) {
			return 900;
		}
		else if (sty_name.contains("extrablack") || sty_name.contains("ultrablack")) {
			return 950;
		}
		return 400;
	}

	_FORCE_INLINE_ int _font_get_stretch_by_name(const String& p_sty_name) const
	{
		String sty_name = p_sty_name.remove_chars(" -");
		if (sty_name.contains("ultracondensed")) {
			return 50;
		}
		else if (sty_name.contains("extracondensed")) {
			return 63;
		}
		else if (sty_name.contains("condensed")) {
			return 75;
		}
		else if (sty_name.contains("semicondensed")) {
			return 87;
		}
		else if (sty_name.contains("semiexpanded")) {
			return 113;
		}
		else if (sty_name.contains("expanded")) {
			return 125;
		}
		else if (sty_name.contains("extraexpanded")) {
			return 150;
		}
		else if (sty_name.contains("ultraexpanded")) {
			return 200;
		}
		return 100;
	}

	_FORCE_INLINE_ bool _is_ital_style(const String& p_sty_name) const
	{
		return p_sty_name.contains("italic") || p_sty_name.contains("oblique");
	}

	// Shaped text cache data.
	struct TrimData
	{
		int trim_pos = -1;
		int ellipsis_pos = -1;
		Vector<Glyph> ellipsis_glyph_buf;
	};

	struct TextRun
	{
		Vector2i range;
		Vector2i gl_range;
		RID font_rid;
		int font_size = 0;
		bool rtl = false;
		int64_t span_index = -1;
	};

	struct ShapedTextDataAdvanced
	{
		Mutex mutex;

		/* Source data */
		RID parent; // Substring parent ShapedTextData.

		int start = 0; // Substring start offset in the parent string.
		int end = 0;   // Substring end offset in the parent string.

		String text;
		String custom_punct;
		TextServer::Direction direction = DIRECTION_LTR; // Desired text direction.
		TextServer::Orientation orientation = ORIENTATION_HORIZONTAL;

		struct Span
		{
			int start = -1;
			int end = -1;

			int font_size = 0;

			String language;
		};

		Vector<Span> spans;
		int first_span = 0; // First span in the parent ShapedTextData.
		int last_span = 0;

		Vector<TextRun> runs;
		bool runs_dirty = true;

		struct EmbeddedObject
		{
			int start = -1;
			int end = -1;
			InlineAlignment inline_align = INLINE_ALIGNMENT_CENTER;
			Rect2 rect;
			double baseline = 0;
		};

		/* Shaped data */
		TextServer::Direction para_direction = DIRECTION_LTR; // Detected text direction.
		int base_para_direction = UBIDI_DEFAULT_LTR;
		SafeFlag valid{false};			// String is shaped.
		bool line_breaks_valid = false; // Line and word break flags are populated (and virtual zero
										// width spaces inserted).
		bool justification_ops_valid = false; // Virtual elongation glyphs are added to the string.
		bool sort_valid = false;
		bool text_trimmed = false;

		bool preserve_invalid = true;  // Draw hex code box instead of missing characters.
		bool preserve_control = false; // Draw control characters.

		double ascent = 0.0;  // Ascent for horizontal layout, 1/2 of width for vertical.
		double descent = 0.0; // Descent for horizontal layout, 1/2 of width for vertical.
		double width = 0.0;	  // Width for horizontal layout, height for vertical.
		double width_trimmed = 0.0;
		int extra_spacing[4] = {0, 0, 0, 0};

		double upos = 0.0;
		double uthk = 0.0;

		char32_t el_char = 0x2026;
		TrimData overrun_trim_data;
		bool fit_width_minimum_reached = false;

		LocalVector<Glyph> glyphs;
		LocalVector<Glyph> glyphs_logical;

		/* Intermediate data */
		Char16String utf16;
		Vector<UBiDi*> bidi_iter;
		Vector<Vector3i> bidi_override;
		ScriptIterator* script_iter = nullptr;
		hb_buffer_t* hb_buffer = nullptr;

		HashMap<int, bool> jstops;
		HashMap<int, bool> breaks;
		PackedInt32Array chars;
		int break_inserts = 0;
		bool break_ops_valid = false;
		bool js_ops_valid = false;
		bool chars_valid = false;

		~ShapedTextDataAdvanced()
		{
			for (int i = 0; i < bidi_iter.size(); i++) {
				if (bidi_iter[i]) {
					ubidi_close(bidi_iter[i]);
				}
			}
			memdelete(script_iter);
			if (hb_buffer) {
				hb_buffer_destroy(hb_buffer);
			}
		}
	};

	// Common data.

	mutable RID_PtrOwner<FontAdvancedLinkedVariation> font_var_owner;
	mutable RID_PtrOwner<FontAdvanced> font_owner;
	mutable RID_PtrOwner<ShapedTextDataAdvanced> shaped_owner{65536, 1048576};

	_FORCE_INLINE_ FontAdvanced* _get_font_data(const RID& p_font_rid) const
	{
		RID rid = p_font_rid;
		FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(rid);
		if (unlikely(fdv)) {
			rid = fdv->base_font;
		}
		return font_owner.get_or_null(rid);
	}

	struct SystemFontKey
	{
		String font_name;
		TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
		bool disable_embedded_bitmaps = true;
		bool italic = false;
		bool mipmaps = false;
		bool msdf = false;
		bool force_autohinter = false;
		int weight = 400;
		int stretch = 100;
		int msdf_range = 14;
		int msdf_source_size = 48;
		int fixed_size = 0;
		TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
		TextServer::SubpixelPositioning subpixel_positioning =
			TextServer::SUBPIXEL_POSITIONING_AUTO;
		bool keep_rounding_remainders = true;
		double embolden = 0.0;
		Transform2D transform;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;

		bool operator==(const SystemFontKey& p_b) const
		{
			return (font_name == p_b.font_name) && (antialiasing == p_b.antialiasing) &&
				   (italic == p_b.italic) &&
				   (disable_embedded_bitmaps == p_b.disable_embedded_bitmaps) &&
				   (mipmaps == p_b.mipmaps) && (msdf == p_b.msdf) &&
				   (force_autohinter == p_b.force_autohinter) && (weight == p_b.weight) &&
				   (stretch == p_b.stretch) && (msdf_range == p_b.msdf_range) &&
				   (msdf_source_size == p_b.msdf_source_size) && (fixed_size == p_b.fixed_size) &&
				   (hinting == p_b.hinting) && (subpixel_positioning == p_b.subpixel_positioning) &&
				   (keep_rounding_remainders == p_b.keep_rounding_remainders) &&
				   (embolden == p_b.embolden) && (transform == p_b.transform) &&
				   (extra_spacing[SPACING_TOP] == p_b.extra_spacing[SPACING_TOP]) &&
				   (extra_spacing[SPACING_BOTTOM] == p_b.extra_spacing[SPACING_BOTTOM]) &&
				   (extra_spacing[SPACING_SPACE] == p_b.extra_spacing[SPACING_SPACE]) &&
				   (extra_spacing[SPACING_GLYPH] == p_b.extra_spacing[SPACING_GLYPH]) &&
				   (baseline_offset == p_b.baseline_offset);
		}

		SystemFontKey(const String& p_font_name, bool p_italic, int p_weight, int p_stretch,
			RID p_font, const TextServerAdvanced* p_fb)
		{
			font_name = p_font_name;
			italic = p_italic;
			weight = p_weight;
			stretch = p_stretch;
		}
	};

	struct SystemFontCacheRec
	{
		RID rid;
		int index = 0;
	};

	struct SystemFontCache
	{
		Vector<SystemFontCacheRec> var;
		int max_var = 0;
	};

	struct SystemFontKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const SystemFontKey& p_a)
		{
			uint32_t hash = p_a.font_name.hash();
			hash = hash_murmur3_one_32(p_a.weight, hash);
			hash = hash_murmur3_one_32(p_a.stretch, hash);
			hash = hash_murmur3_one_32(p_a.msdf_range, hash);
			hash = hash_murmur3_one_32(p_a.msdf_source_size, hash);
			hash = hash_murmur3_one_32(p_a.fixed_size, hash);
			hash = hash_murmur3_one_double(p_a.embolden, hash);
			hash = hash_murmur3_one_real(p_a.transform[0].x, hash);
			hash = hash_murmur3_one_real(p_a.transform[0].y, hash);
			hash = hash_murmur3_one_real(p_a.transform[1].x, hash);
			hash = hash_murmur3_one_real(p_a.transform[1].y, hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_TOP], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_BOTTOM], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_SPACE], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_GLYPH], hash);
			hash = hash_murmur3_one_double(p_a.baseline_offset, hash);
			return hash_fmix32(hash_murmur3_one_32(
				((int)p_a.mipmaps) | ((int)p_a.msdf << 1) | ((int)p_a.italic << 2) |
					((int)p_a.force_autohinter << 3) | ((int)p_a.hinting << 4) |
					((int)p_a.subpixel_positioning << 8) | ((int)p_a.antialiasing << 12) |
					((int)p_a.disable_embedded_bitmaps << 14) |
					((int)p_a.keep_rounding_remainders << 15),
				hash));
		}
	};

	mutable HashMap<SystemFontKey, SystemFontCache, SystemFontKeyHasher> system_fonts;
	mutable HashMap<String, PackedByteArray> system_font_data;

	void _update_chars(ShapedTextDataAdvanced* p_sd) const;
	void _generate_runs(ShapedTextDataAdvanced* p_sd) const;
	void _realign(ShapedTextDataAdvanced* p_sd) const;
	int64_t _convert_pos(const String& p_utf32, const Char16String& p_utf16, int64_t p_pos) const;
	int64_t _convert_pos(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const;
	int64_t _convert_pos_inv(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const;
	bool _shape_substr(ShapedTextDataAdvanced* p_new_sd, const ShapedTextDataAdvanced* p_sd,
		int64_t p_start, int64_t p_length) const;

	struct FontPriorityList
	{
		friend class TextServerAdvanced;

		const int PRIORITY_SKIP = 100; // Font already used.
		const int PRIORITY_MAX = 2;
		int current_priority = 0;
		uint32_t current_index = 0;
		uint32_t font_count = 0;
		String language;
		String script_code;
		bool color = false;
		LocalVector<Pair<RID, int>> unprocessed_fonts;
		LocalVector<RID> fonts;
		const TextServerAdvanced* text_server;

		_FORCE_INLINE_ uint32_t size() const { return font_count; }

		RID operator[](uint32_t p_index)
		{
			if (p_index < fonts.size()) {
				return fonts[p_index];
			}
			while (current_priority < PRIORITY_MAX || current_index < font_count) {
				if (current_index >= font_count) {
					current_priority++;
					current_index = 0;
				}
				const RID& font = unprocessed_fonts[current_index].first;
				int& priority = unprocessed_fonts[current_index].second;
				if (priority < 0) {
					priority = 0;
				}
				if (priority == current_priority) {
					unprocessed_fonts[current_index].second = PRIORITY_SKIP;
					fonts.push_back(font);
					if (p_index < fonts.size()) {
						return fonts[p_index];
					}
				}
				current_index++;
			}
			return RID();
		}
	};

	void _shape_run(ShapedTextDataAdvanced* p_sd, int64_t p_start, int64_t p_end,
		const String& p_language, hb_script_t p_script, hb_direction_t p_direction,
		FontPriorityList& p_fonts, int64_t p_span, int64_t p_fb_index, int64_t p_prev_start,
		int64_t p_prev_end, RID p_prev_font);
	Glyph _shape_single_glyph(ShapedTextDataAdvanced* p_sd, char32_t p_char, hb_script_t p_script,
		hb_direction_t p_direction, const RID& p_font, int64_t p_font_size);
	_FORCE_INLINE_ RID _find_sys_font_for_text(const RID& p_fdef, const String& p_script_code,
		const String& p_language, const String& p_text);

	String os_locale;

	Mutex ft_mutex;

	// HarfBuzz bitmap font interface.

	static hb_font_funcs_t* funcs;

	struct bmp_font_t
	{
		TextServerAdvanced::FontForSizeAdvanced* face = nullptr;
		bool unref = false; /* Whether to destroy bm_face when done. */
	};

	static bmp_font_t* _bmp_font_create(
		TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref);
	static void _bmp_font_destroy(void* p_data);
	static hb_bool_t _bmp_get_nominal_glyph(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_unicode, hb_codepoint_t* r_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_h_advance(
		hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_v_advance(
		hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_h_kerning(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_left_glyph, hb_codepoint_t p_right_glyph, void* p_user_data);
	static hb_bool_t _bmp_get_glyph_v_origin(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_glyph, hb_position_t* r_x, hb_position_t* r_y, void* p_user_data);
	static hb_bool_t _bmp_get_glyph_extents(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_glyph, hb_glyph_extents_t* r_extents, void* p_user_data);
	static hb_bool_t _bmp_get_font_h_extents(
		hb_font_t* p_font, void* p_font_data, hb_font_extents_t* r_metrics, void* p_user_data);
	static void _bmp_create_font_funcs();
	static void _bmp_free_font_funcs();
	static void _bmp_font_set_funcs(
		hb_font_t* p_font, TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref);
	static hb_font_t* _bmp_font_create(
		TextServerAdvanced::FontForSizeAdvanced* p_face, hb_destroy_func_t p_destroy);

	hb_font_t* _font_get_hb_handle(const RID& p_font, int64_t p_font_size, bool& r_is_color) const;
	bool _font_is_color(const RID& p_font) const;

	struct GlyphCompare
	{ // For line breaking reordering.

		_FORCE_INLINE_ bool operator()(const Glyph& l, const Glyph& r) const
		{
			if (l.start == r.start) {
				if (l.count == r.count) {
					return (l.flags & TextServer::GRAPHEME_IS_VIRTUAL) <
						   (r.flags & TextServer::GRAPHEME_IS_VIRTUAL);
				}
				return l.count > r.count; // Sort first glyph with count & flags, order of the rest
										  // are irrelevant.
			}
			else {
				return l.start < r.start;
			}
		}
	};

protected:
	void full_copy(ShapedTextDataAdvanced* p_shaped);
	void invalidate(ShapedTextDataAdvanced* p_shaped, bool p_text = false);

public:
	TextServerAdvanced() = default;
	bool _has_feature(Feature p_feature) const;
	String _get_name() const;
	String _get_short_name() const;
	int64_t _get_features() const;
	void _free_rid(const RID& p_rid);
	bool _has(const RID& p_rid);
	bool _load_support_data(const String& p_filename);
	String _get_support_data_filename() const;
	String _get_support_data_info() const;
	bool _save_support_data(const String& p_filename) const;
	PackedByteArray _get_support_data() const;
	bool _is_locale_using_support_data(const String& p_locale) const;
	bool _is_locale_right_to_left(const String& p_locale) const;
	int64_t _name_to_tag(const String& p_name) const;
	String _tag_to_name(int64_t p_tag) const;

#ifdef MODULE_MSDFGEN_ENABLED

	struct MSContext
	{
		msdfgen::Point2 position;
		msdfgen::Shape* shape = nullptr;
		msdfgen::Contour* contour = nullptr;
	};

	class DistancePixelConversion
	{
		double invRange;

	public:
		_FORCE_INLINE_ explicit DistancePixelConversion(double range) : invRange(1 / range) {}

		_FORCE_INLINE_ void operator()(
			float* pixels, const msdfgen::MultiAndTrueDistance& distance) const
		{
			pixels[0] = float(invRange * distance.r + .5);
			pixels[1] = float(invRange * distance.g + .5);
			pixels[2] = float(invRange * distance.b + .5);
			pixels[3] = float(invRange * distance.a + .5);
		}
	};

	struct MSDFThreadData
	{
		msdfgen::Bitmap<float, 4>* output;
		msdfgen::Shape* shape;
		msdfgen::Projection* projection;
		DistancePixelConversion* distancePixelConversion;
	};

	static msdfgen::Point2 ft_point2(const FT_Vector& vector);
	static int ft_move_to(const FT_Vector* to, void* user);
	static int ft_line_to(const FT_Vector* to, void* user);
	static int ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user);
	static int ft_cubic_to(
		const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user);

#endif

	/*************************************************************************/
	/* Font Cache                                                            */
	/*************************************************************************/

	void _reference_oversampling_level(double p_oversampling);
	void _unreference_oversampling_level(double p_oversampling);
	RID _create_font();
	RID _create_font_linked_variation(const RID& p_font_rid);
	void _font_set_data(const RID& p_font_rid, const PackedByteArray& p_data);
	void _font_set_data_ptr(const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size);
	void _font_set_face_index(const RID& p_font_rid, int64_t p_face_index);
	int64_t _font_get_face_index(const RID& p_font_rid) const;
	int64_t _font_get_face_count(const RID& p_font_rid) const;
	void _font_set_style(const RID& p_font_rid, uint32_t p_style);
	uint32_t _font_get_style(const RID& p_font_rid) const;
	void _font_set_style_name(const RID& p_font_rid, const String& p_name);
	String _font_get_style_name(const RID& p_font_rid) const;
	void _font_set_weight(const RID& p_font_rid, int64_t p_weight);
	int64_t _font_get_weight(const RID& p_font_rid) const;
	void _font_set_stretch(const RID& p_font_rid, int64_t p_stretch);
	int64_t _font_get_stretch(const RID& p_font_rid) const;
	void _font_set_name(const RID& p_font_rid, const String& p_name);
	void _font_set_antialiasing(const RID& p_font_rid, TextServer::FontAntialiasing p_antialiasing);
	TextServer::FontAntialiasing _font_get_antialiasing(const RID& p_font_rid) const;
	void _font_set_disable_embedded_bitmaps(const RID& p_font_rid, bool p_disable_embedded_bitmaps);
	bool _font_get_disable_embedded_bitmaps(const RID& p_font_rid) const;
	void _font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps);
	bool _font_get_generate_mipmaps(const RID& p_font_rid) const;
	void _font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf);
	bool _font_is_multichannel_signed_distance_field(const RID& p_font_rid) const;
	void _font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range);
	int64_t _font_get_msdf_pixel_range(const RID& p_font_rid) const;
	void _font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size);
	int64_t _font_get_msdf_size(const RID& p_font_rid) const;
	void _font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size);
	int64_t _font_get_fixed_size(const RID& p_font_rid) const;
	void _font_set_fixed_size_scale_mode(
		const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode);
	TextServer::FixedSizeScaleMode _font_get_fixed_size_scale_mode(const RID& p_font_rid) const;
	void _font_set_allow_system_fallback(const RID& p_font_rid, bool p_allow_system_fallback);
	bool _font_is_allow_system_fallback(const RID& p_font_rid) const;
	void _font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter);
	bool _font_is_force_autohinter(const RID& p_font_rid) const;
	void _font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate);
	bool _font_is_modulate_color_glyphs(const RID& p_font_rid) const;
	int64_t _font_get_palette_count(const RID& p_font_rid) const;
	String _font_get_palette_name(const RID& p_font_rid, int64_t p_index) const;
	Vector<Color> _font_get_palette_colors(const RID& p_font_rid, int64_t p_index) const;
	void _font_set_palette_custom_colors(const RID& p_font_rid, const Vector<Color>& p_colors);
	Vector<Color> _font_get_palette_custom_colors(const RID& p_font_rid) const;
	int64_t _font_get_used_palette(const RID& p_font_rid) const;
	void _font_set_used_palette(const RID& p_font_rid, int64_t p_index);
	void _font_set_hinting(const RID& p_font_rid, TextServer::Hinting p_hinting);
	TextServer::Hinting _font_get_hinting(const RID& p_font_rid) const;
	void _font_set_subpixel_positioning(
		const RID& p_font_rid, TextServer::SubpixelPositioning p_subpixel);
	TextServer::SubpixelPositioning _font_get_subpixel_positioning(const RID& p_font_rid) const;
	void _font_set_keep_rounding_remainders(const RID& p_font_rid, bool p_keep_rounding_remainders);
	bool _font_get_keep_rounding_remainders(const RID& p_font_rid) const;
	void _font_set_embolden(const RID& p_font_rid, double p_strength);
	double _font_get_embolden(const RID& p_font_rid) const;
	void _font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value);
	int64_t _font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const;
	void _font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset);
	double _font_get_baseline_offset(const RID& p_font_rid) const;
	void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) override;
	Transform2D _font_get_transform(const RID& p_font_rid) const;
	double _font_get_oversampling(const RID& p_font_rid) const;
	void _font_set_oversampling(const RID& p_font_rid, double p_oversampling);
	TypedArray<Vector2i> _font_get_size_cache_list(const RID& p_font_rid) const;
	void _font_clear_size_cache(const RID& p_font_rid);
	void _font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size);
	void _font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent);
	double _font_get_ascent(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent);
	double _font_get_descent(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position);
	double _font_get_underline_position(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness);
	double _font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale);
	double _font_get_scale(const RID& p_font_rid, int64_t p_size) const;
	int64_t _font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const;
	void _font_clear_textures(const RID& p_font_rid, const Vector2i& p_size);
	void _font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index);
	void _font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image);
	Ref<Image> _font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const;
	void _font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const PackedInt32Array& p_offsets);
	PackedInt32Array _font_get_texture_offsets(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const;
	PackedInt32Array _font_get_glyph_list(const RID& p_font_rid, const Vector2i& p_size) const;
	void _font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size);
	void _font_remove_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph);
	Vector2 _font_get_glyph_advance(const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const;
	void _font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance);
	Vector2 _font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset);
	Vector2 _font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size);
	Rect2 _font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect);
	int64_t _font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx);
	RID _font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	Size2 _font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	TypedArray<Vector2i> _font_get_kerning_list(const RID& p_font_rid, int64_t p_size) const;
	void _font_clear_kerning_map(const RID& p_font_rid, int64_t p_size);
	void _font_remove_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair);
	void _font_set_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair,
		const Vector2& p_kerning);
	Vector2 _font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const;
	int64_t _font_get_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const;
	int64_t _font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const;
	bool _font_has_char(const RID& p_font_rid, int64_t p_char) const;
	String _font_get_supported_chars(const RID& p_font_rid) const;
	PackedInt32Array _font_get_supported_glyphs(const RID& p_font_rid) const;
	void _font_render_range(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_start, int64_t p_end);
	void _font_render_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_index);
	void _font_draw_glyph(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color, float p_oversampling) const;
	void _font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const;
	bool _font_is_language_supported(const RID& p_font_rid, const String& p_language) const;
	void _font_set_language_support_override(
		const RID& p_font_rid, const String& p_language, bool p_supported);
	bool _font_get_language_support_override(const RID& p_font_rid, const String& p_language);
	void _font_remove_language_support_override(const RID& p_font_rid, const String& p_language);
	PackedStringArray _font_get_language_support_overrides(const RID& p_font_rid);
	bool _font_is_script_supported(const RID& p_font_rid, const String& p_script) const;
	void _font_set_script_support_override(
		const RID& p_font_rid, const String& p_script, bool p_supported);
	bool _font_get_script_support_override(const RID& p_font_rid, const String& p_script);
	void _font_remove_script_support_override(const RID& p_font_rid, const String& p_script);
	PackedStringArray _font_get_script_support_overrides(const RID& p_font_rid);
	RID _create_shaped_text(
		TextServer::Direction p_direction, TextServer::Orientation p_orientation);
	void _shaped_text_clear(const RID& p_shaped);
	RID _shaped_text_duplicate(const RID& p_shaped);
	void _shaped_text_set_direction(const RID& p_shaped, TextServer::Direction p_direction);
	TextServer::Direction _shaped_text_get_direction(const RID& p_shaped) const;
	TextServer::Direction _shaped_text_get_inferred_direction(const RID& p_shaped) const;
	void _shaped_text_set_custom_punctuation(const RID& p_shaped, const String& p_punct);
	String _shaped_text_get_custom_punctuation(const RID& p_shaped) const;
	void _shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char);
	int64_t _shaped_text_get_custom_ellipsis(const RID& p_shaped) const;
	void _shaped_text_set_orientation(const RID& p_shaped, TextServer::Orientation p_orientation);
	void _shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled);
	bool _shaped_text_get_preserve_invalid(const RID& p_shaped) const;
	void _shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled);
	bool _shaped_text_get_preserve_control(const RID& p_shaped) const;
	void _shaped_text_set_spacing(const RID& p_shaped, SpacingType p_spacing, int64_t p_value);
	int64_t _shaped_text_get_spacing(const RID& p_shaped, SpacingType p_spacing) const;
	TextServer::Orientation _shaped_text_get_orientation(const RID& p_shaped) const;
	int64_t _shaped_get_span_count(const RID& p_shaped) const;
	String _shaped_get_span_text(const RID& p_shaped, int64_t p_index) const;
	int64_t _shaped_get_run_count(const RID& p_shaped) const;
	String _shaped_get_run_text(const RID& p_shaped, int64_t p_index) const;
	Vector2i _shaped_get_run_range(const RID& p_shaped, int64_t p_index) const;
	Vector2i _shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const;
	RID _shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const;
	int _shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const;
	String _shaped_get_run_language(const RID& p_shaped, int64_t p_index) const;
	TextServer::Direction _shaped_get_run_direction(const RID& p_shaped, int64_t p_index) const;
	String _shaped_get_text(const RID& p_shaped) const;
	RID _shaped_text_substr(const RID& p_shaped, int64_t p_start, int64_t p_length) const;
	RID _shaped_text_get_parent(const RID& p_shaped) const;
	double _shaped_text_fit_to_width(const RID& p_shaped, double p_width, uint32_t p_jst_flags);
	double _shaped_text_tab_align(const RID& p_shaped, const PackedFloat32Array& p_tab_stops);
	void _shaped_text_overrun_trim_to_width(
		const RID& p_shaped_line, double p_width, uint32_t p_trim_flags);
	int64_t _shaped_text_get_trim_pos(const RID& p_shaped) const;
	int64_t _shaped_text_get_ellipsis_pos(const RID& p_shaped) const;
	const Glyph* _shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const;
	int64_t _shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const;
	PackedInt32Array _shaped_text_get_character_breaks(const RID& p_shaped) const;
	bool _shaped_text_update_breaks(const RID& p_shaped);
	_FORCE_INLINE_ int64_t _generate_kashida_justification_opportunities(
		const String& p_data, int64_t p_start, int64_t p_end);
	bool _shaped_text_update_justification_ops(const RID& p_shaped);
	bool _shaped_text_shape(const RID& p_shaped);
	bool _shaped_text_is_ready(const RID& p_shaped) const;
	const Glyph* _shaped_text_get_glyphs(const RID& p_shaped) const;
	int64_t _shaped_text_get_glyph_count(const RID& p_shaped) const;
	const Glyph* _shaped_text_sort_logical(const RID& p_shaped);
	Vector2i _shaped_text_get_range(const RID& p_shaped) const;
	Size2 _shaped_text_get_size(const RID& p_shaped) const;
	double _shaped_text_get_ascent(const RID& p_shaped) const;
	double _shaped_text_get_descent(const RID& p_shaped) const;
	double _shaped_text_get_width(const RID& p_shaped) const;
	double _shaped_text_get_underline_position(const RID& p_shaped) const;
	double _shaped_text_get_underline_thickness(const RID& p_shaped) const;
	int64_t _is_confusable(const String& p_string, const PackedStringArray& p_dict) const;
	bool _spoof_check(const String& p_string) const;
	String _strip_diacritics(const String& p_string) const;
	String _string_to_upper(const String& p_string, const String& p_language) const;
	String _string_to_lower(const String& p_string, const String& p_language) const;
	String _string_to_title(const String& p_string, const String& p_language) const;
	PackedInt32Array _string_get_word_breaks(
		const String& p_string, const String& p_language, int64_t p_chars_per_line) const;
	PackedInt32Array _string_get_character_breaks(
		const String& p_string, const String& p_language) const;
	bool _is_valid_identifier(const String& p_string) const;
	bool _is_valid_letter(uint64_t p_unicode) const;
	void _font_clear_system_fallback_cache();
	String _font_get_name(const RID& p_font_rid) const;
	void _cleanup();

	// Virtual TextServer overrides forwarding to TextServerAdvanced implementations
	bool has_feature(Feature p_feature) const override { return _has_feature(p_feature); }
	String get_name() const override { return _get_name(); }
	String get_short_name() const override { return _get_short_name(); }
	int64_t get_features() const override { return _get_features(); }
	void free_rid(const RID& p_rid) override { _free_rid(p_rid); }
	bool has(const RID& p_rid) override { return _has(p_rid); }
	bool load_support_data(const String& p_filename) override
	{
		return _load_support_data(p_filename);
	}
	String get_support_data_filename() const override { return _get_support_data_filename(); }
	String get_support_data_info() const override { return _get_support_data_info(); }
	bool save_support_data(const String& p_filename) const override
	{
		return _save_support_data(p_filename);
	}
	PackedByteArray get_support_data() const override { return _get_support_data(); }
	bool is_locale_using_support_data(const String& p_locale) const override
	{
		return _is_locale_using_support_data(p_locale);
	}
	bool is_locale_right_to_left(const String& p_locale) const override
	{
		return _is_locale_right_to_left(p_locale);
	}
	int64_t name_to_tag(const String& p_name) const override { return _name_to_tag(p_name); }
	String tag_to_name(int64_t p_tag) const override { return _tag_to_name(p_tag); }

	void reference_oversampling_level(double p_oversampling) override
	{
		_reference_oversampling_level(p_oversampling);
	}
	void unreference_oversampling_level(double p_oversampling) override
	{
		_unreference_oversampling_level(p_oversampling);
	}

	RID create_font() override { return _create_font(); }
	RID create_font_linked_variation(const RID& p_font_rid) override
	{
		return _create_font_linked_variation(p_font_rid);
	}
	void font_set_data(const RID& p_font_rid, const PackedByteArray& p_data) override
	{
		_font_set_data(p_font_rid, p_data);
	}
	void font_set_data_ptr(
		const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size) override
	{
		_font_set_data_ptr(p_font_rid, p_data_ptr, p_data_size);
	}
	void font_set_face_index(const RID& p_font_rid, int64_t p_index) override
	{
		_font_set_face_index(p_font_rid, p_index);
	}
	int64_t font_get_face_index(const RID& p_font_rid) const override
	{
		return _font_get_face_index(p_font_rid);
	}
	int64_t font_get_face_count(const RID& p_font_rid) const override
	{
		return _font_get_face_count(p_font_rid);
	}
	void font_set_style(const RID& p_font_rid, uint32_t p_style) override
	{
		_font_set_style(p_font_rid, p_style);
	}
	uint32_t font_get_style(const RID& p_font_rid) const override
	{
		return _font_get_style(p_font_rid);
	}
	void font_set_name(const RID& p_font_rid, const String& p_name) override
	{
		_font_set_name(p_font_rid, p_name);
	}
	String font_get_name(const RID& p_font_rid) const override
	{
		return _font_get_name(p_font_rid);
	}
	void font_set_style_name(const RID& p_font_rid, const String& p_name) override
	{
		_font_set_style_name(p_font_rid, p_name);
	}
	String font_get_style_name(const RID& p_font_rid) const override
	{
		return _font_get_style_name(p_font_rid);
	}
	void font_set_weight(const RID& p_font_rid, int64_t p_weight) override
	{
		_font_set_weight(p_font_rid, p_weight);
	}
	int64_t font_get_weight(const RID& p_font_rid) const override
	{
		return _font_get_weight(p_font_rid);
	}
	void font_set_stretch(const RID& p_font_rid, int64_t p_stretch) override
	{
		_font_set_stretch(p_font_rid, p_stretch);
	}
	int64_t font_get_stretch(const RID& p_font_rid) const override
	{
		return _font_get_stretch(p_font_rid);
	}
	void font_set_antialiasing(const RID& p_font_rid, FontAntialiasing p_antialiasing) override
	{
		_font_set_antialiasing(p_font_rid, p_antialiasing);
	}
	FontAntialiasing font_get_antialiasing(const RID& p_font_rid) const override
	{
		return _font_get_antialiasing(p_font_rid);
	}
	void font_set_disable_embedded_bitmaps(
		const RID& p_font_rid, bool p_disable_embedded_bitmaps) override
	{
		_font_set_disable_embedded_bitmaps(p_font_rid, p_disable_embedded_bitmaps);
	}
	bool font_get_disable_embedded_bitmaps(const RID& p_font_rid) const override
	{
		return _font_get_disable_embedded_bitmaps(p_font_rid);
	}
	void font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps) override
	{
		_font_set_generate_mipmaps(p_font_rid, p_generate_mipmaps);
	}
	bool font_get_generate_mipmaps(const RID& p_font_rid) const override
	{
		return _font_get_generate_mipmaps(p_font_rid);
	}
	void font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf) override
	{
		_font_set_multichannel_signed_distance_field(p_font_rid, p_msdf);
	}
	bool font_is_multichannel_signed_distance_field(const RID& p_font_rid) const override
	{
		return _font_is_multichannel_signed_distance_field(p_font_rid);
	}
	void font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range) override
	{
		_font_set_msdf_pixel_range(p_font_rid, p_msdf_pixel_range);
	}
	int64_t font_get_msdf_pixel_range(const RID& p_font_rid) const override
	{
		return _font_get_msdf_pixel_range(p_font_rid);
	}
	void font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size) override
	{
		_font_set_msdf_size(p_font_rid, p_msdf_size);
	}
	int64_t font_get_msdf_size(const RID& p_font_rid) const override
	{
		return _font_get_msdf_size(p_font_rid);
	}
	void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) override
	{
		_font_set_fixed_size(p_font_rid, p_fixed_size);
	}
	int64_t font_get_fixed_size(const RID& p_font_rid) const override
	{
		return _font_get_fixed_size(p_font_rid);
	}
	void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, FixedSizeScaleMode p_fixed_size_scale) override
	{
		_font_set_fixed_size_scale_mode(p_font_rid, p_fixed_size_scale);
	}
	FixedSizeScaleMode font_get_fixed_size_scale_mode(const RID& p_font_rid) const override
	{
		return _font_get_fixed_size_scale_mode(p_font_rid);
	}
	void font_set_allow_system_fallback(
		const RID& p_font_rid, bool p_allow_system_fallback) override
	{
		_font_set_allow_system_fallback(p_font_rid, p_allow_system_fallback);
	}
	bool font_is_allow_system_fallback(const RID& p_font_rid) const override
	{
		return _font_is_allow_system_fallback(p_font_rid);
	}
	void font_clear_system_fallback_cache() override { _font_clear_system_fallback_cache(); }
	void font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter) override
	{
		_font_set_force_autohinter(p_font_rid, p_force_autohinter);
	}
	bool font_is_force_autohinter(const RID& p_font_rid) const override
	{
		return _font_is_force_autohinter(p_font_rid);
	}
	void font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate) override
	{
		_font_set_modulate_color_glyphs(p_font_rid, p_modulate);
	}
	bool font_is_modulate_color_glyphs(const RID& p_font_rid) const override
	{
		return _font_is_modulate_color_glyphs(p_font_rid);
	}
	void font_set_hinting(const RID& p_font_rid, Hinting p_hinting) override
	{
		_font_set_hinting(p_font_rid, p_hinting);
	}
	Hinting font_get_hinting(const RID& p_font_rid) const override
	{
		return _font_get_hinting(p_font_rid);
	}
	void font_set_subpixel_positioning(
		const RID& p_font_rid, SubpixelPositioning p_subpixel) override
	{
		_font_set_subpixel_positioning(p_font_rid, p_subpixel);
	}
	SubpixelPositioning font_get_subpixel_positioning(const RID& p_font_rid) const override
	{
		return _font_get_subpixel_positioning(p_font_rid);
	}
	void font_set_keep_rounding_remainders(
		const RID& p_font_rid, bool p_keep_rounding_remainders) override
	{
		_font_set_keep_rounding_remainders(p_font_rid, p_keep_rounding_remainders);
	}
	bool font_get_keep_rounding_remainders(const RID& p_font_rid) const override
	{
		return _font_get_keep_rounding_remainders(p_font_rid);
	}
	void font_set_embolden(const RID& p_font_rid, double p_strength) override
	{
		_font_set_embolden(p_font_rid, p_strength);
	}
	double font_get_embolden(const RID& p_font_rid) const override
	{
		return _font_get_embolden(p_font_rid);
	}
	void font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value) override
	{
		_font_set_spacing(p_font_rid, p_spacing, p_value);
	}
	int64_t font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const override
	{
		return _font_get_spacing(p_font_rid, p_spacing);
	}
	void font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset) override
	{
		_font_set_baseline_offset(p_font_rid, p_baseline_offset);
	}
	double font_get_baseline_offset(const RID& p_font_rid) const override
	{
		return _font_get_baseline_offset(p_font_rid);
	}
	Transform2D font_get_transform(const RID& p_font_rid) const override
	{
		return _font_get_transform(p_font_rid);
	}
	void font_set_oversampling(const RID& p_font_rid, double p_oversampling) override
	{
		_font_set_oversampling(p_font_rid, p_oversampling);
	}
	double font_get_oversampling(const RID& p_font_rid) const override
	{
		return _font_get_oversampling(p_font_rid);
	}
	void font_clear_size_cache(const RID& p_font_rid) override
	{
		_font_clear_size_cache(p_font_rid);
	}
	void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) override
	{
		_font_remove_size_cache(p_font_rid, p_size);
	}
	void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) override
	{
		_font_set_ascent(p_font_rid, p_size, p_ascent);
	}
	double font_get_ascent(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_ascent(p_font_rid, p_size);
	}
	void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) override
	{
		_font_set_descent(p_font_rid, p_size, p_descent);
	}
	double font_get_descent(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_descent(p_font_rid, p_size);
	}
	void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position) override
	{
		_font_set_underline_position(p_font_rid, p_size, p_underline_position);
	}
	double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_underline_position(p_font_rid, p_size);
	}
	void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness) override
	{
		_font_set_underline_thickness(p_font_rid, p_size, p_underline_thickness);
	}
	double font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_underline_thickness(p_font_rid, p_size);
	}
	void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) override
	{
		_font_set_scale(p_font_rid, p_size, p_scale);
	}
	double font_get_scale(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_scale(p_font_rid, p_size);
	}
	int64_t font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return _font_get_texture_count(p_font_rid, p_size);
	}
	void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) override
	{
		_font_clear_textures(p_font_rid, p_size);
	}
	void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) override
	{
		_font_remove_texture(p_font_rid, p_size, p_texture_index);
	}
	void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image) override
	{
		_font_set_texture_image(p_font_rid, p_size, p_texture_index, p_image);
	}
	Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const override
	{
		return _font_get_texture_image(p_font_rid, p_size, p_texture_index);
	}
	Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_advance(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_advance(const RID& p_font_rid, int64_t p_size, int64_t p_glyph,
		const Vector2& p_advance) override
	{
		_font_set_glyph_advance(p_font_rid, p_size, p_glyph, p_advance);
	}
	Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_offset(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_offset(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_offset) override
	{
		_font_set_glyph_offset(p_font_rid, p_size, p_glyph, p_offset);
	}
	Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_size(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_size(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_gl_size) override
	{
		_font_set_glyph_size(p_font_rid, p_size, p_glyph, p_gl_size);
	}
	Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_uv_rect(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_uv_rect(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Rect2& p_uv_rect) override
	{
		_font_set_glyph_uv_rect(p_font_rid, p_size, p_glyph, p_uv_rect);
	}
	int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_idx(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_texture_idx(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		int64_t p_texture_idx) override
	{
		_font_set_glyph_texture_idx(p_font_rid, p_size, p_glyph, p_texture_idx);
	}
	RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_rid(p_font_rid, p_size, p_glyph);
	}
	Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_size(p_font_rid, p_size, p_glyph);
	}
	void font_clear_kerning_map(const RID& p_font_rid, int64_t p_size) override
	{
		_font_clear_kerning_map(p_font_rid, p_size);
	}
	void font_remove_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) override
	{
		_font_remove_kerning(p_font_rid, p_size, p_glyph_pair);
	}
	void font_set_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair,
		const Vector2& p_kerning) override
	{
		_font_set_kerning(p_font_rid, p_size, p_glyph_pair, p_kerning);
	}
	Vector2 font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const override
	{
		return _font_get_kerning(p_font_rid, p_size, p_glyph_pair);
	}
	int64_t font_get_glyph_index(const RID& p_font_rid, int64_t p_size, int64_t p_char,
		int64_t p_variation_selector) const override
	{
		return _font_get_glyph_index(p_font_rid, p_size, p_char, p_variation_selector);
	}
	int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const override
	{
		return _font_get_char_from_glyph_index(p_font_rid, p_size, p_glyph_index);
	}
	bool font_has_char(const RID& p_font_rid, int64_t p_char) const override
	{
		return _font_has_char(p_font_rid, p_char);
	}
	String font_get_supported_chars(const RID& p_font_rid) const override
	{
		return _font_get_supported_chars(p_font_rid);
	}
	PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const override
	{
		return _font_get_supported_glyphs(p_font_rid);
	}
	void font_render_range(
		const RID& p_font, const Vector2i& p_size, int64_t p_start, int64_t p_end) override
	{
		_font_render_range(p_font, p_size, p_start, p_end);
	}
	void font_render_glyph(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_index) override
	{
		_font_render_glyph(p_font_rid, p_size, p_index);
	}
	void font_draw_glyph(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const override
	{
		_font_draw_glyph(p_font, p_canvas, p_size, p_pos, p_index, p_color, p_oversampling);
	}
	void font_draw_glyph_outline(const RID& p_font, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override
	{
		_font_draw_glyph_outline(
			p_font, p_canvas, p_size, p_outline_size, p_pos, p_index, p_color, p_oversampling);
	}
	bool font_is_language_supported(
		const RID& p_font_rid, const String& p_language) const override
	{
		return _font_is_language_supported(p_font_rid, p_language);
	}
	bool font_is_script_supported(const RID& p_font_rid, const String& p_script) const override
	{
		return _font_is_script_supported(p_font_rid, p_script);
	}
	void cleanup() override { _cleanup(); }

	~TextServerAdvanced();
};

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED

#include <freetype/freetype.h>
#include <freetype/otsvg.h>
#include <ft2build.h>
#include <thorvg.h>

struct GL_State {
	bool ready = false;
	float x = 0;
	float y = 0;
	float w = 0;
	float h = 0;
	CharString xml_code;
	tvg::Matrix m;
};

struct TVG_NodeCache {
	uint64_t document_offset;
	uint64_t body_offset;
};

struct TVG_DocumentCache {
	String xml_body;
	double embox_x;
	double embox_y;
	HashMap<int64_t, Vector<TVG_NodeCache>> node_caches;
};

struct TVG_State {
	Mutex mutex;
	HashMap<uint32_t, GL_State> glyph_map;
	HashMap<FT_Byte *, TVG_DocumentCache> document_map;
};

FT_Error tvg_svg_in_ot_init(FT_Pointer *p_state);
void tvg_svg_in_ot_free(FT_Pointer *p_state);
FT_Error tvg_svg_in_ot_preset_slot(FT_GlyphSlot p_slot, FT_Bool p_cache, FT_Pointer *p_state);
FT_Error tvg_svg_in_ot_render(FT_GlyphSlot p_slot, FT_Pointer *p_state);

SVG_RendererHooks *get_tvg_svg_in_ot_hooks();

#endif // MODULE_FREETYPE_ENABLED
#endif // MODULE_SVG_ENABLED

// --- End modules/text_server_adv/text_server_adv.h ---

// Built-in ICU data.

#ifdef ICU_STATIC_DATA
#include <icudata.gen.h>
#endif

// Thirdparty headers.

#ifdef MODULE_MSDFGEN_ENABLED
VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wshadow")
VLTR_MSVC_WARNING_PUSH_AND_IGNORE(4458) // "Declaration of 'identifier' hides class member".

#include <core/EdgeHolder.h>
#include <core/ShapeDistanceFinder.h>
#include <core/contour-combiners.h>
#include <core/edge-selectors.h>
#include <msdfgen.h>

VLTR_GCC_WARNING_POP
VLTR_MSVC_WARNING_POP
#endif

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED
#endif
#endif

/*************************************************************************/
/*  bmp_font_t HarfBuzz Bitmap font interface                            */
/*************************************************************************/

hb_font_funcs_t* TextServerAdvanced::funcs = nullptr;

TextServerAdvanced::bmp_font_t* TextServerAdvanced::_bmp_font_create(
	TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref)
{
	bmp_font_t* bm_font = memnew(bmp_font_t);

	if (!bm_font) {
		return nullptr;
	}

	bm_font->face = p_face;
	bm_font->unref = p_unref;

	return bm_font;
}

void TextServerAdvanced::_bmp_font_destroy(void* p_data)
{
	bmp_font_t* bm_font = static_cast<bmp_font_t*>(p_data);
	memdelete(bm_font);
}

hb_bool_t TextServerAdvanced::_bmp_get_nominal_glyph(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_unicode, hb_codepoint_t* r_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	if (!bm_font->face->glyph_map.has(p_unicode)) {
		if (bm_font->face->glyph_map.has(0xf000u + p_unicode)) {
			*r_glyph = 0xf000u + p_unicode;
			return true;
		}
		else {
			return false;
		}
	}

	*r_glyph = p_unicode;
	return true;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_h_advance(
	hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return 0;
	}

	return E->value.advance.x * 64;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_v_advance(
	hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return 0;
	}

	return -E->value.advance.y * 64;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_h_kerning(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_left_glyph, hb_codepoint_t p_right_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	if (!bm_font->face->kerning_map.has(Vector2i(p_left_glyph, p_right_glyph))) {
		return 0;
	}

	return bm_font->face->kerning_map[Vector2i(p_left_glyph, p_right_glyph)].x * 64;
}

hb_bool_t TextServerAdvanced::_bmp_get_glyph_v_origin(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_glyph, hb_position_t* r_x, hb_position_t* r_y, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return false;
	}

	*r_x = E->value.advance.x * 32;
	*r_y = -bm_font->face->ascent * 64;

	return true;
}

hb_bool_t TextServerAdvanced::_bmp_get_glyph_extents(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_glyph, hb_glyph_extents_t* r_extents, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return false;
	}

	r_extents->x_bearing = 0;
	r_extents->y_bearing = 0;
	r_extents->width = E->value.rect.size.x * 64;
	r_extents->height = E->value.rect.size.y * 64;

	return true;
}

hb_bool_t TextServerAdvanced::_bmp_get_font_h_extents(
	hb_font_t* p_font, void* p_font_data, hb_font_extents_t* r_metrics, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	r_metrics->ascender = bm_font->face->ascent;
	r_metrics->descender = bm_font->face->descent;
	r_metrics->line_gap = 0;

	return true;
}

void TextServerAdvanced::_bmp_create_font_funcs()
{
	if (funcs == nullptr) {
		funcs = hb_font_funcs_create();

		hb_font_funcs_set_font_h_extents_func(funcs, _bmp_get_font_h_extents, nullptr, nullptr);
		hb_font_funcs_set_nominal_glyph_func(funcs, _bmp_get_nominal_glyph, nullptr, nullptr);
		hb_font_funcs_set_glyph_h_advance_func(funcs, _bmp_get_glyph_h_advance, nullptr, nullptr);
		hb_font_funcs_set_glyph_v_advance_func(funcs, _bmp_get_glyph_v_advance, nullptr, nullptr);
		hb_font_funcs_set_glyph_v_origin_func(funcs, _bmp_get_glyph_v_origin, nullptr, nullptr);
		hb_font_funcs_set_glyph_h_kerning_func(funcs, _bmp_get_glyph_h_kerning, nullptr, nullptr);
		hb_font_funcs_set_glyph_extents_func(funcs, _bmp_get_glyph_extents, nullptr, nullptr);

		hb_font_funcs_make_immutable(funcs);
	}
}

void TextServerAdvanced::_bmp_free_font_funcs()
{
	if (funcs != nullptr) {
		hb_font_funcs_destroy(funcs);
		funcs = nullptr;
	}
}

void TextServerAdvanced::_bmp_font_set_funcs(
	hb_font_t* p_font, TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref)
{
	hb_font_set_funcs(p_font, funcs, _bmp_font_create(p_face, p_unref), _bmp_font_destroy);
}

hb_font_t* TextServerAdvanced::_bmp_font_create(
	TextServerAdvanced::FontForSizeAdvanced* p_face, hb_destroy_func_t p_destroy)
{
	hb_font_t* font;
	hb_face_t* face = hb_face_create(nullptr, 0);

	font = hb_font_create(face);
	hb_face_destroy(face);
	_bmp_font_set_funcs(font, p_face, false);
	return font;
}

/*************************************************************************/
/*  Character properties.                                                */
/*************************************************************************/

_FORCE_INLINE_ bool is_ain(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AIN;
}

_FORCE_INLINE_ bool is_alef(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_ALEF;
}

_FORCE_INLINE_ bool is_beh(char32_t p_chr)
{
	int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
	return (prop == U_JG_BEH) || (prop == U_JG_NOON) || (prop == U_JG_AFRICAN_NOON) ||
		   (prop == U_JG_NYA) || (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH);
}

_FORCE_INLINE_ bool is_dal(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_DAL;
}

_FORCE_INLINE_ bool is_feh(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_FEH) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_FEH);
}

_FORCE_INLINE_ bool is_gaf(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_GAF;
}

_FORCE_INLINE_ bool is_heh(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_HEH;
}

_FORCE_INLINE_ bool is_kaf(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_KAF;
}

_FORCE_INLINE_ bool is_lam(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_LAM;
}

_FORCE_INLINE_ bool is_qaf(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_QAF) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_QAF);
}

_FORCE_INLINE_ bool is_reh(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_REH;
}

_FORCE_INLINE_ bool is_seen_sad(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SAD) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SEEN);
}

_FORCE_INLINE_ bool is_tah(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TAH;
}

_FORCE_INLINE_ bool is_teh_marbuta(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TEH_MARBUTA;
}

_FORCE_INLINE_ bool is_yeh(char32_t p_chr)
{
	int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
	return (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH) || (prop == U_JG_YEH_BARREE) ||
		   (prop == U_JG_BURUSHASKI_YEH_BARREE) || (prop == U_JG_YEH_WITH_TAIL);
}

_FORCE_INLINE_ bool is_waw(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_WAW;
}

_FORCE_INLINE_ bool is_transparent(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_TYPE) == U_JT_TRANSPARENT;
}

_FORCE_INLINE_ bool is_ligature(char32_t p_chr, char32_t p_nchr)
{
	return (is_lam(p_chr) && is_alef(p_nchr));
}

_FORCE_INLINE_ bool is_connected_to_prev(char32_t p_chr, char32_t p_pchr)
{
	int32_t prop = u_getIntPropertyValue(p_pchr, UCHAR_JOINING_TYPE);
	return (prop != U_JT_RIGHT_JOINING) && (prop != U_JT_NON_JOINING) ? !is_ligature(p_pchr, p_chr)
																	  : false;
}

/*************************************************************************/

bool TextServerAdvanced::icu_data_loaded = false;
PackedByteArray TextServerAdvanced::icu_data;

bool TextServerAdvanced::_has_feature(Feature p_feature) const
{
	switch (p_feature) {
	case FEATURE_SIMPLE_LAYOUT:
	case FEATURE_BIDI_LAYOUT:
	case FEATURE_VERTICAL_LAYOUT:
	case FEATURE_SHAPING:
	case FEATURE_KASHIDA_JUSTIFICATION:
	case FEATURE_BREAK_ITERATORS:
	case FEATURE_FONT_BITMAP:
#ifdef MODULE_FREETYPE_ENABLED
	case FEATURE_FONT_DYNAMIC:
#endif
#ifdef MODULE_MSDFGEN_ENABLED
	case FEATURE_FONT_MSDF:
#endif
	case FEATURE_FONT_VARIABLE:
	case FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION:
	case FEATURE_USE_SUPPORT_DATA:
	case FEATURE_UNICODE_IDENTIFIERS:
	case FEATURE_UNICODE_SECURITY:
		return true;
	default: {
	}
	}
	return false;
}

String TextServerAdvanced::_get_name() const { return "ICU / HarfBuzz / Graphite (Built-in)"; }

String TextServerAdvanced::_get_short_name() const { return "advanced"; }

int64_t TextServerAdvanced::_get_features() const
{
	int64_t interface_features =
		FEATURE_SIMPLE_LAYOUT | FEATURE_BIDI_LAYOUT | FEATURE_VERTICAL_LAYOUT | FEATURE_SHAPING |
		FEATURE_KASHIDA_JUSTIFICATION | FEATURE_BREAK_ITERATORS | FEATURE_FONT_BITMAP |
		FEATURE_FONT_VARIABLE | FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION |
		FEATURE_USE_SUPPORT_DATA;
#ifdef MODULE_FREETYPE_ENABLED
	interface_features |= FEATURE_FONT_DYNAMIC;
#endif
#ifdef MODULE_MSDFGEN_ENABLED
	interface_features |= FEATURE_FONT_MSDF;
#endif

	return interface_features;
}

void TextServerAdvanced::_free_rid(const RID& p_rid)
{
	_THREAD_SAFE_METHOD_
	if (font_owner.owns(p_rid)) {
		MutexLock ftlock(ft_mutex);

		FontAdvanced* fd = font_owner.get_or_null(p_rid);
		for (const KeyValue<Vector2i, FontForSizeAdvanced*>& ffsd : fd->cache) {
			OversamplingLevel* ol = oversampling_levels.getptr(ffsd.value->viewport_oversampling);
			if (ol != nullptr) {
				ol->fonts.erase(ffsd.value);
			}
		}
		{
			MutexLock lock(fd->mutex);
			font_owner.free(p_rid);
		}
		memdelete(fd);
	}
	else if (font_var_owner.owns(p_rid)) {
		MutexLock ftlock(ft_mutex);

		FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_rid);
		{
			font_var_owner.free(p_rid);
		}
		memdelete(fdv);
	}
	else if (shaped_owner.owns(p_rid)) {
		ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_rid);
		{
			MutexLock lock(sd->mutex);
			shaped_owner.free(p_rid);
		}
		memdelete(sd);
	}
}

bool TextServerAdvanced::_has(const RID& p_rid)
{
	_THREAD_SAFE_METHOD_
	return font_owner.owns(p_rid) || font_var_owner.owns(p_rid) || shaped_owner.owns(p_rid);
}

bool TextServerAdvanced::_load_support_data(const String& p_filename)
{
	_THREAD_SAFE_METHOD_

#if defined(ICU_STATIC_DATA) || !defined(HAVE_ICU_BUILTIN)
	if (!icu_data_loaded) {
		UErrorCode err = U_ZERO_ERROR;
		u_init(&err); // Do not check for errors, since we only load part of the data.
		icu_data_loaded = true;
	}
#else
	if (!icu_data_loaded) {
		UErrorCode err = U_ZERO_ERROR;
		String filename = (p_filename.is_empty()) ? String("res://icudt_godot.dat") : p_filename;
		if (FileAccess::exists(filename)) {
			Ref<FileAccess> f = FileAccess::open(filename, FileAccess::READ);
			if (f.is_null()) {
				return false;
			}
			uint64_t len = f->get_length();
			icu_data = f->get_buffer(len);

			udata_setCommonData(icu_data.ptr(), &err);
			if (U_FAILURE(err)) {
				ERR_FAIL_V_MSG(false, u_errorName(err));
			}

			err = U_ZERO_ERROR;
			icu_data_loaded = true;
		}

		u_init(&err);
		if (U_FAILURE(err)) {
			ERR_FAIL_V_MSG(false, u_errorName(err));
		}
	}
#endif
	return true;
}

String TextServerAdvanced::_get_support_data_filename() const { return String("icudt_godot.dat"); }

String TextServerAdvanced::_get_support_data_info() const
{
	return String("ICU break iteration data (\"icudt_godot.dat\").");
}

bool TextServerAdvanced::_save_support_data(const String& p_filename) const
{
	_THREAD_SAFE_METHOD_
#ifdef ICU_STATIC_DATA

	// Store data to the res file if it's available.

	Ref<FileAccess> f = FileAccess::open(p_filename, FileAccess::WRITE);
	if (f.is_null()) {
		return false;
	}

	PackedByteArray icu_data_static;
	icu_data_static.resize(U_ICUDATA_SIZE);
	memcpy(icu_data_static.ptrw(), U_ICUDATA_ENTRY_POINT, U_ICUDATA_SIZE);
	f->store_buffer(icu_data_static);

	return true;
#else
	return false;
#endif
}

PackedByteArray TextServerAdvanced::_get_support_data() const
{
	_THREAD_SAFE_METHOD_
#ifdef ICU_STATIC_DATA

	PackedByteArray icu_data_static;
	icu_data_static.resize(U_ICUDATA_SIZE);
	memcpy(icu_data_static.ptrw(), U_ICUDATA_ENTRY_POINT, U_ICUDATA_SIZE);

	return icu_data_static;
#else
	return icu_data;
#endif
}

bool TextServerAdvanced::_is_locale_using_support_data(const String& p_locale) const
{
	String l = p_locale.get_slicec('_', 0);
	if ((l == "my") || (l == "zh") || (l == "ja") || (l == "ko") || (l == "km") || (l == "lo") ||
		(l == "th")) {
		return true;
	}
	else {
		return false;
	}
}

bool TextServerAdvanced::_is_locale_right_to_left(const String& p_locale) const
{
	String l = p_locale.get_slicec('_', 0);
	if ((l == "ar") || (l == "dv") || (l == "he") || (l == "fa") || (l == "ff") || (l == "ku") ||
		(l == "ur")) {
		return true;
	}
	else {
		return false;
	}
}

int64_t TextServerAdvanced::_name_to_tag(const String& p_name) const
{
	if (feature_sets.has(p_name)) {
		return feature_sets[p_name];
	}

	// No readable name, use tag string.
	return hb_tag_from_string(p_name.replace("custom_", "").ascii().get_data(), -1);
}

bool TextServerAdvanced::_get_tag_hidden(int64_t p_tag) const
{
	if (feature_sets_inv.has(p_tag)) {
		return feature_sets_inv[p_tag].hidden;
	}
	return false;
}

String TextServerAdvanced::_tag_to_name(int64_t p_tag) const
{
	if (feature_sets_inv.has(p_tag)) {
		return feature_sets_inv[p_tag].name;
	}

	// No readable name, use tag string.
	char name[5];
	memset(name, 0, 5);
	hb_tag_to_string(p_tag, name);
	return String("custom_") + String(name);
}

/*************************************************************************/
/* Font Glyph Rendering                                                  */
/*************************************************************************/

_FORCE_INLINE_ TextServerAdvanced::FontTexturePosition
TextServerAdvanced::find_texture_pos_for_glyph(FontForSizeAdvanced* p_data, int p_color_size,
	Image::Format p_image_format, int p_width, int p_height, bool p_msdf) const
{
	FontTexturePosition ret;

	int mw = p_width;
	int mh = p_height;

	ShelfPackTexture* ct = p_data->textures.ptrw();
	for (int32_t i = 0; i < p_data->textures.size(); i++) {
		if (ct[i].image.is_null()) {
			continue;
		}
		if (p_image_format != ct[i].image->get_format()) {
			continue;
		}
		if (mw > ct[i].texture_w || mh > ct[i].texture_h) { // Too big for this texture.
			continue;
		}

		ret = ct[i].pack_rect(i, mh, mw);
		if (ret.index != -1) {
			break;
		}
	}

	if (ret.index == -1) {
		// Could not find texture to fit, create one.
		int texsize = MAX(p_data->size.x * 0.125, 256);

		texsize = Math::next_power_of_2((uint32_t)texsize);
		if (p_msdf) {
			texsize = MIN(texsize, 2048);
		}
		else {
			texsize = MIN(texsize, 1024);
		}
		if (mw > texsize) { // Special case, adapt to it?
			texsize = Math::next_power_of_2((uint32_t)mw);
		}
		if (mh > texsize) { // Special case, adapt to it?
			texsize = Math::next_power_of_2((uint32_t)mh);
		}

		ShelfPackTexture tex = ShelfPackTexture(texsize, texsize);
		tex.image = Image::create_empty(texsize, texsize, false, p_image_format);
		{
			// Zero texture.
			uint8_t* w = tex.image->ptrw();
			ERR_FAIL_COND_V(texsize * texsize * p_color_size > tex.image->get_data_size(), ret);
			// Initialize the texture to all-white pixels to prevent artifacts when the
			// font is displayed at a non-default scale with filtering enabled.
			if (p_color_size == 2) {
				for (int i = 0; i < texsize * texsize * p_color_size;
					 i += 2) { // FORMAT_LA8, BW font.
					w[i + 0] = 255;
					w[i + 1] = 0;
				}
			}
			else if (p_color_size == 4) {
				for (int i = 0; i < texsize * texsize * p_color_size;
					 i += 4) { // FORMAT_RGBA8, Color font, Multichannel(+True) SDF.
					if (p_msdf) {
						w[i + 0] = 0;
						w[i + 1] = 0;
						w[i + 2] = 0;
					}
					else {
						w[i + 0] = 255;
						w[i + 1] = 255;
						w[i + 2] = 255;
					}
					w[i + 3] = 0;
				}
			}
			else {
				ERR_FAIL_V(ret);
			}
		}
		p_data->textures.push_back(tex);

		int32_t idx = p_data->textures.size() - 1;
		ret = p_data->textures.write[idx].pack_rect(idx, mh, mw);
	}

	return ret;
}

#ifdef MODULE_MSDFGEN_ENABLED

struct MSContext
{
	msdfgen::Point2 position;
	msdfgen::Shape* shape = nullptr;
	msdfgen::Contour* contour = nullptr;
};

class DistancePixelConversion
{
	double invRange;

public:
	_FORCE_INLINE_ explicit DistancePixelConversion(double range) : invRange(1 / range) {}

	_FORCE_INLINE_ void operator()(
		float* pixels, const msdfgen::MultiAndTrueDistance& distance) const
	{
		pixels[0] = float(invRange * distance.r + .5);
		pixels[1] = float(invRange * distance.g + .5);
		pixels[2] = float(invRange * distance.b + .5);
		pixels[3] = float(invRange * distance.a + .5);
	}
};

struct MSDFThreadData
{
	msdfgen::Bitmap<float, 4>* output;
	msdfgen::Shape* shape;
	msdfgen::Projection* projection;
	DistancePixelConversion* distancePixelConversion;
};

// static msdfgen::Point2 ft_point2(const FT_Vector &vector) {
// 	return msdfgen::Point2(vector.x / 60.0f, vector.y / 60.0f);
// }

// static int ft_move_to(const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	if (!(context->contour && context->contour->edges.empty())) {
// 		context->contour = &context->shape->addContour();
// 	}
// 	context->position = ft_point2(*to);
// 	return 0;
// }

// static int ft_line_to(const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	msdfgen::Point2 endpoint = ft_point2(*to);
// 	if (endpoint != context->position) {
// 		context->contour->addEdge(new msdfgen::LinearSegment(context->position, endpoint));
// 		context->position = endpoint;
// 	}
// 	return 0;
// }

// static int ft_conic_to(const FT_Vector *control, const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	context->contour->addEdge(new msdfgen::QuadraticSegment(context->position, ft_point2(*control),
// ft_point2(*to))); 	context->position = ft_point2(*to); 	return 0;
// }

// static int ft_cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to,
// void *user) { 	MSContext *context = static_cast<MSContext *>(user);
// context->contour->addEdge(new msdfgen::CubicSegment(context->position, ft_point2(*control1),
// ft_point2(*control2), ft_point2(*to))); 	context->position = ft_point2(*to); 	return 0;
// }

void TextServerAdvanced::_generateMTSDF_threaded(void* p_td, uint32_t p_y)
{
	MSDFThreadData* td = static_cast<MSDFThreadData*>(p_td);

	msdfgen::ShapeDistanceFinder<
		msdfgen::OverlappingContourCombiner<msdfgen::MultiAndTrueDistanceSelector>>
		distanceFinder(*td->shape);
	int row = td->shape->inverseYAxis ? td->output->height() - p_y - 1 : p_y;
	for (int col = 0; col < td->output->width(); ++col) {
		int x = (p_y % 2) ? td->output->width() - col - 1 : col;
		msdfgen::Point2 p = td->projection->unproject(msdfgen::Point2(x + .5, p_y + .5));
		msdfgen::MultiAndTrueDistance distance = distanceFinder.distance(p);
		td->distancePixelConversion->operator()(td->output->operator()(x, row), distance);
	}
}

#endif

#ifdef MODULE_FREETYPE_ENABLED
#if HB_VERSION_ATLEAST(13, 0, 0)
_FORCE_INLINE_ TextServerAdvanced::FontGlyph TextServerAdvanced::rasterize_hb_bitmap(
	FontForSizeAdvanced* p_data, int p_rect_margin, hb_raster_image_t* p_image,
	const hb_raster_extents_t& p_ext, const Vector2& p_advance, bool p_bgra) const
{
	FontGlyph chr;
	chr.advance = p_advance * p_data->scale;
	chr.found = true;

	int w = p_ext.width;
	int h = p_ext.height;

	if (w == 0 || h == 0 || p_image == nullptr) {
		chr.texture_idx = -1;
		chr.uv_rect = Rect2();
		chr.rect = Rect2();
		return chr;
	}

	int color_size = p_bgra ? 4 : 2;

	int mw = w + p_rect_margin * 4;
	int mh = h + p_rect_margin * 4;

	ERR_FAIL_COND_V(mw > 4096, FontGlyph());
	ERR_FAIL_COND_V(mh > 4096, FontGlyph());

	Image::Format require_format = color_size == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8;

	FontTexturePosition tex_pos =
		find_texture_pos_for_glyph(p_data, color_size, require_format, mw, mh, false);
	ERR_FAIL_COND_V(tex_pos.index < 0, FontGlyph());

	const uint8_t* img_src = hb_raster_image_get_buffer(p_image);

	// Fit character in char texture.
	ShelfPackTexture& tex = p_data->textures.write[tex_pos.index];

	{
		uint8_t* wr = tex.image->ptrw();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				int ofs = ((i + tex_pos.y + p_rect_margin * 2) * tex.texture_w + j + tex_pos.x +
							  p_rect_margin * 2) *
						  color_size;
				ERR_FAIL_COND_V(ofs >= tex.image->get_data_size(), FontGlyph());
				if (p_bgra) {
					int ofs_color = i * p_ext.stride + (j << 2);
					wr[ofs + 2] = img_src[ofs_color + 0];
					wr[ofs + 1] = img_src[ofs_color + 1];
					wr[ofs + 0] = img_src[ofs_color + 2];
					wr[ofs + 3] = img_src[ofs_color + 3];
				}
				else {
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = img_src[i * p_ext.stride + j];
				}
			}
		}
	}

	tex.dirty = true;

	chr.texture_idx = tex_pos.index;

	chr.uv_rect = Rect2(tex_pos.x + p_rect_margin, tex_pos.y + p_rect_margin, w + p_rect_margin * 2,
		h + p_rect_margin * 2);
	chr.rect.position =
		Vector2(p_ext.x_origin - p_rect_margin, p_ext.y_origin - p_rect_margin) * p_data->scale;
	chr.rect.size = chr.uv_rect.size * p_data->scale;
	return chr;
}
#endif

_FORCE_INLINE_ TextServerAdvanced::FontGlyph TextServerAdvanced::rasterize_bitmap(
	FontForSizeAdvanced* p_data, int p_rect_margin, FT_Bitmap p_bitmap, int p_yofs, int p_xofs,
	const Vector2& p_advance, bool p_bgra) const
{
	FontGlyph chr;
	chr.advance = p_advance * p_data->scale;
	chr.found = true;

	int w = p_bitmap.width;
	int h = p_bitmap.rows;

	if (w == 0 || h == 0 || p_bitmap.buffer == nullptr) {
		chr.texture_idx = -1;
		chr.uv_rect = Rect2();
		chr.rect = Rect2();
		return chr;
	}

	int color_size = 2;

	switch (p_bitmap.pixel_mode) {
	case FT_PIXEL_MODE_MONO:
	case FT_PIXEL_MODE_GRAY: {
		color_size = 2;
	} break;
	case FT_PIXEL_MODE_BGRA: {
		color_size = 4;
	} break;
	case FT_PIXEL_MODE_LCD: {
		color_size = 4;
		w /= 3;
	} break;
	case FT_PIXEL_MODE_LCD_V: {
		color_size = 4;
		h /= 3;
	} break;
	}

	int mw = w + p_rect_margin * 4;
	int mh = h + p_rect_margin * 4;

	ERR_FAIL_COND_V(mw > 4096, FontGlyph());
	ERR_FAIL_COND_V(mh > 4096, FontGlyph());

	Image::Format require_format = color_size == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8;

	FontTexturePosition tex_pos =
		find_texture_pos_for_glyph(p_data, color_size, require_format, mw, mh, false);
	ERR_FAIL_COND_V(tex_pos.index < 0, FontGlyph());

	// Fit character in char texture.
	ShelfPackTexture& tex = p_data->textures.write[tex_pos.index];

	{
		uint8_t* wr = tex.image->ptrw();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				int ofs = ((i + tex_pos.y + p_rect_margin * 2) * tex.texture_w + j + tex_pos.x +
							  p_rect_margin * 2) *
						  color_size;
				ERR_FAIL_COND_V(ofs >= tex.image->get_data_size(), FontGlyph());
				switch (p_bitmap.pixel_mode) {
				case FT_PIXEL_MODE_MONO: {
					int byte = i * p_bitmap.pitch + (j >> 3);
					int bit = 1 << (7 - (j % 8));
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = (p_bitmap.buffer[byte] & bit) ? 255 : 0;
				} break;
				case FT_PIXEL_MODE_GRAY:
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = p_bitmap.buffer[i * p_bitmap.pitch + j];
					break;
				case FT_PIXEL_MODE_BGRA: {
					int ofs_color = i * p_bitmap.pitch + (j << 2);
					wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
					wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
					wr[ofs + 0] = p_bitmap.buffer[ofs_color + 2];
					wr[ofs + 3] = p_bitmap.buffer[ofs_color + 3];
				} break;
				case FT_PIXEL_MODE_LCD: {
					int ofs_color = i * p_bitmap.pitch + (j * 3);
					if (p_bgra) {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 2];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 3] = 255;
					}
					else {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 2];
						wr[ofs + 3] = 255;
					}
				} break;
				case FT_PIXEL_MODE_LCD_V: {
					int ofs_color = i * p_bitmap.pitch * 3 + j;
					if (p_bgra) {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + p_bitmap.pitch * 2];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + p_bitmap.pitch];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 3] = 255;
					}
					else {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + p_bitmap.pitch];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + p_bitmap.pitch * 2];
						wr[ofs + 3] = 255;
					}
				} break;
				default:
					ERR_FAIL_V_MSG(FontGlyph(), "Font uses unsupported pixel format: " +
													String::num_int64(p_bitmap.pixel_mode) + ".");
					break;
				}
			}
		}
	}

	tex.dirty = true;

	chr.texture_idx = tex_pos.index;

	chr.uv_rect = Rect2(tex_pos.x + p_rect_margin, tex_pos.y + p_rect_margin, w + p_rect_margin * 2,
		h + p_rect_margin * 2);
	chr.rect.position = Vector2(p_xofs - p_rect_margin, -p_yofs - p_rect_margin) * p_data->scale;
	chr.rect.size = chr.uv_rect.size * p_data->scale;
	return chr;
}
#endif

/*************************************************************************/
/* Font Cache                                                            */
/*************************************************************************/

bool TextServerAdvanced::_ensure_glyph(FontAdvanced* p_font_data, const Vector2i& p_size,
	int32_t p_glyph, FontGlyph& r_glyph, uint32_t p_oversampling) const
{
	FontForSizeAdvanced* fd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(p_font_data, p_size, fd, false, p_oversampling), false);

	int32_t glyph_index = p_glyph & 0xffffff; // Remove subpixel shifts.

	HashMap<int32_t, FontGlyph>::Iterator E = fd->glyph_map.find(p_glyph);
	if (E) {
		bool tx_valid = true;
		if (E->value.texture_idx >= 0) {
			if (E->value.texture_idx < fd->textures.size()) {
				tx_valid = fd->textures[E->value.texture_idx].image.is_valid();
			}
			else {
				tx_valid = false;
			}
		}
		if (tx_valid) {
			r_glyph = E->value;
			return E->value.found;
#ifdef DEBUG_ENABLED
		}
		else {
			WARN_PRINT(vformat("Invalid texture cache for glyph %x in font %s, glyph will be "
							   "re-rendered. Re-import this font to regenerate textures.",
				glyph_index, p_font_data->font_name));
#endif
		}
	}

	if (glyph_index == 0) { // Non graphical or invalid glyph, do not render.
		E = fd->glyph_map.insert(p_glyph, FontGlyph());
		r_glyph = E->value;
		return true;
	}

#ifdef MODULE_FREETYPE_ENABLED
	FontGlyph gl;
	if (p_font_data->face) {
		FT_Int32 flags = FT_LOAD_DEFAULT;

		bool outline = p_size.y > 0;
		switch (p_font_data->hinting) {
		case TextServer::HINTING_NONE:
			flags |= FT_LOAD_NO_HINTING;
			break;
		case TextServer::HINTING_LIGHT:
			flags |= FT_LOAD_TARGET_LIGHT;
			break;
		default:
			flags |= FT_LOAD_TARGET_NORMAL;
			break;
		}
		if (p_font_data->force_autohinter) {
			flags |= FT_LOAD_FORCE_AUTOHINT;
		}
		if (outline ||
			(p_font_data->disable_embedded_bitmaps && !FT_HAS_COLOR(p_font_data->face))) {
			flags |= FT_LOAD_NO_BITMAP;
		}
		else if (FT_HAS_COLOR(p_font_data->face)) {
			flags |= FT_LOAD_COLOR;
		}

		FT_Fixed v, h;
		FT_Get_Advance(p_font_data->face, glyph_index, flags, &h);
		FT_Get_Advance(p_font_data->face, glyph_index, flags | FT_LOAD_VERTICAL_LAYOUT, &v);

		int error = FT_Load_Glyph(p_font_data->face, glyph_index, flags);
		if (error) {
			E = fd->glyph_map.insert(p_glyph, FontGlyph());
			r_glyph = E->value;
			return false;
		}

		if (!p_font_data->msdf) {
			if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
				(p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					p_size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
				FT_Pos xshift = (int)((p_glyph >> 27) & 3) << 4;
				FT_Outline_Translate(&p_font_data->face->glyph->outline, xshift, 0);
			}
			else if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
					   (p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						   p_size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
				FT_Pos xshift = (int)((p_glyph >> 27) & 3) << 5;
				FT_Outline_Translate(&p_font_data->face->glyph->outline, xshift, 0);
			}
		}

		if (p_font_data->embolden != 0.f) {
			FT_Pos strength =
				p_font_data->embolden * p_size.x / 16; // 26.6 fractional units (1 / 64).
			FT_Outline_Embolden(&p_font_data->face->glyph->outline, strength);
		}

		if (p_font_data->transform != Transform2D()) {
			FT_Matrix mat = {FT_Fixed(p_font_data->transform[0][0] * 65536),
				FT_Fixed(p_font_data->transform[0][1] * 65536),
				FT_Fixed(p_font_data->transform[1][0] * 65536),
				FT_Fixed(
					p_font_data->transform[1][1] * 65536)}; // 16.16 fractional units (1 / 65536).
			FT_Outline_Transform(&p_font_data->face->glyph->outline, &mat);
		}

		FT_Render_Mode aa_mode = FT_RENDER_MODE_NORMAL;
		bool bgra = false;
		switch (p_font_data->antialiasing) {
		case FONT_ANTIALIASING_NONE: {
			aa_mode = FT_RENDER_MODE_MONO;
		} break;
		case FONT_ANTIALIASING_GRAY: {
			aa_mode = FT_RENDER_MODE_NORMAL;
		} break;
		case FONT_ANTIALIASING_LCD: {
			int aa_layout = (int)((p_glyph >> 24) & 7);
			switch (aa_layout) {
			case FONT_LCD_SUBPIXEL_LAYOUT_HRGB: {
				aa_mode = FT_RENDER_MODE_LCD;
				bgra = false;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_HBGR: {
				aa_mode = FT_RENDER_MODE_LCD;
				bgra = true;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_VRGB: {
				aa_mode = FT_RENDER_MODE_LCD_V;
				bgra = false;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_VBGR: {
				aa_mode = FT_RENDER_MODE_LCD_V;
				bgra = true;
			} break;
			default: {
				aa_mode = FT_RENDER_MODE_NORMAL;
			} break;
			}
		} break;
		}

		FT_GlyphSlot slot = p_font_data->face->glyph;
		bool fix_edge =
			(slot->format == FT_GLYPH_FORMAT_SVG); // Need to check before FT_Render_Glyph as it
												   // will change format to bitmap.
#if HB_VERSION_ATLEAST(13, 0, 0)
		bool from_bitmap = (slot->format == FT_GLYPH_FORMAT_BITMAP);
#endif
		if (!outline) {
			if (p_font_data->msdf) {
#ifndef MODULE_MSDFGEN_ENABLED
				fd->glyph_map[p_glyph] = FontGlyph();
				ERR_FAIL_V_MSG(false, "Compiled without MSDFGEN support!");
#endif
			}
			else {
#if HB_VERSION_ATLEAST(13, 0, 0)
				if (p_font_data->hb_rdr && p_font_data->hb_mono &&
					p_font_data->antialiasing == FONT_ANTIALIASING_GRAY && !from_bitmap) {
					bool is_rasterized = false;
					float xshift = 0.0;
					if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
						(p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							p_size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
						xshift = float((int)((p_glyph >> 27) & 3) << 4);
					}
					else if ((p_font_data->subpixel_positioning ==
								   SUBPIXEL_POSITIONING_ONE_HALF) ||
							   (p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
								   p_size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
						xshift = float((int)((p_glyph >> 27) & 3) << 5);
					}
					xshift += (p_font_data->embolden * double(p_size.x) / 64.0);
					if (fd->color_paint) {
						hb_raster_paint_reset(p_font_data->hb_rdr);
						hb_raster_paint_set_scale_factor(p_font_data->hb_rdr, 64.0, 64.0);
						if (Math::is_equal_approx(p_font_data->transform[0][0], (real_t)1.f) &&
							Math::is_equal_approx(p_font_data->transform[1][0], (real_t)0.f) &&
							Math::is_equal_approx(p_font_data->transform[1][1], (real_t)1.f)) {
							hb_raster_paint_set_transform(
								p_font_data->hb_rdr, 1.f, 0.f, 0.f, -1.f, xshift, 0.f);
						}
						else {
							Transform2D tr = p_font_data->transform * Transform2D::FLIP_Y;
							hb_raster_paint_set_transform(p_font_data->hb_rdr, tr[0][0], tr[1][0],
								tr[0][1], tr[1][1], xshift, 0.f);
						}
						hb_raster_paint_clear_custom_palette_colors(p_font_data->hb_rdr);
						if (!p_font_data->palette_custom_colors_hb.is_empty()) {
							for (int col = 0; col < p_font_data->palette_custom_colors_hb.size();
								 col++) {
								if (p_font_data->palette_custom_colors_hb[col] != 0) {
									hb_raster_paint_set_custom_palette_color(p_font_data->hb_rdr,
										col, p_font_data->palette_custom_colors_hb[col]);
								}
							}
						}
#if HB_VERSION_ATLEAST(14, 2, 0)
						hb_raster_paint_set_palette(
							p_font_data->hb_rdr, p_font_data->palette_index);
						bool ok = hb_raster_paint_glyph_or_fail(
							p_font_data->hb_rdr, fd->hb_handle, (hb_codepoint_t)glyph_index);
#else
						bool ok = hb_raster_paint_glyph(p_font_data->hb_rdr, fd->hb_handle,
							(hb_codepoint_t)glyph_index, 0, 0, p_font_data->palette_index,
							(hb_color_t)0xFFFFFFFF);
#endif
						if (ok) {
							is_rasterized = true;
							fix_edge = false;
							hb_raster_image_t* img = hb_raster_paint_render(p_font_data->hb_rdr);
							hb_raster_extents_t ext = {0, 0, 0, 0, 0};
							if (img) {
								hb_raster_image_get_extents(img, &ext);
								gl = rasterize_hb_bitmap(fd, rect_range, img, ext,
									Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0,
									true);
								hb_raster_paint_recycle_image(p_font_data->hb_rdr, img);
							}
							else {
								gl = rasterize_hb_bitmap(fd, rect_range, nullptr, ext,
									Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0,
									true);
							}
						}
					}
					if (!is_rasterized && !fix_edge &&
						p_font_data->hinting == TextServer::HINTING_NONE) {
						hb_raster_draw_reset(p_font_data->hb_mono);
						hb_raster_draw_set_scale_factor(p_font_data->hb_mono, 64.0, 64.0);
						if (Math::is_equal_approx(p_font_data->transform[0][0], (real_t)1.f) &&
							Math::is_equal_approx(p_font_data->transform[1][0], (real_t)0.f) &&
							Math::is_equal_approx(p_font_data->transform[1][1], (real_t)1.f)) {
							hb_raster_draw_set_transform(
								p_font_data->hb_mono, 1.f, 0.f, 0.f, -1.f, xshift, 0.f);
						}
						else {
							Transform2D tr = p_font_data->transform * Transform2D::FLIP_Y;
							hb_raster_draw_set_transform(p_font_data->hb_mono, tr[0][0], tr[1][0],
								tr[0][1], tr[1][1], xshift, 0.f);
						}
#if HB_VERSION_ATLEAST(14, 2, 0)
						hb_raster_draw_glyph(
							p_font_data->hb_mono, fd->hb_handle, (hb_codepoint_t)glyph_index);
#else
						hb_raster_draw_glyph(
							p_font_data->hb_mono, fd->hb_handle, (hb_codepoint_t)glyph_index, 0, 0);
#endif
						hb_raster_image_t* img = hb_raster_draw_render(p_font_data->hb_mono);
						hb_raster_extents_t ext = {0, 0, 0, 0, 0};
						if (img) {
							is_rasterized = true;
							hb_raster_image_get_extents(img, &ext);
							gl = rasterize_hb_bitmap(fd, rect_range, img, ext,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, false);
							hb_raster_draw_recycle_image(p_font_data->hb_mono, img);
						}
						else {
							gl = rasterize_hb_bitmap(fd, rect_range, nullptr, ext,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, false);
						}
					}
					if (!is_rasterized) {
						error = FT_Render_Glyph(slot, aa_mode);
						if (!error) {
							gl = rasterize_bitmap(fd, rect_range, slot->bitmap, slot->bitmap_top,
								slot->bitmap_left,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, bgra);
						}
					}
				}
				else {
#else
				{
#endif
					error = FT_Render_Glyph(slot, aa_mode);
					if (!error) {
						gl = rasterize_bitmap(fd, rect_range, slot->bitmap, slot->bitmap_top,
							slot->bitmap_left,
							Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, bgra);
					}
				}
			}
		}
		else {
			FT_Stroker stroker;
			if (FT_Stroker_New(ft_library, &stroker) != 0) {
				fd->glyph_map[p_glyph] = FontGlyph();
				ERR_FAIL_V_MSG(false, "FreeType: Failed to load glyph stroker.");
			}

			FT_Stroker_Set(stroker, (int)(fd->size.y * 16.0), FT_STROKER_LINECAP_BUTT,
				FT_STROKER_LINEJOIN_ROUND, 0);
			FT_Glyph glyph;
			FT_BitmapGlyph glyph_bitmap;

			if (FT_Get_Glyph(p_font_data->face->glyph, &glyph) != 0) {
				goto cleanup_stroker;
			}
			if (FT_Glyph_Stroke(&glyph, stroker, 1) != 0) {
				goto cleanup_glyph;
			}
			if (FT_Glyph_To_Bitmap(&glyph, aa_mode, nullptr, 1) != 0) {
				goto cleanup_glyph;
			}
			glyph_bitmap = (FT_BitmapGlyph)glyph;
			gl = rasterize_bitmap(fd, rect_range, glyph_bitmap->bitmap, glyph_bitmap->top,
				glyph_bitmap->left, Vector2(), bgra);

		cleanup_glyph:
			FT_Done_Glyph(glyph);
		cleanup_stroker:
			FT_Stroker_Done(stroker);
		}
		gl.fix_edge = fix_edge;
		E = fd->glyph_map.insert(p_glyph, gl);
		r_glyph = E->value;
		return gl.found;
	}
#endif
	E = fd->glyph_map.insert(p_glyph, FontGlyph());
	r_glyph = E->value;
	return false;
}

void TextServerAdvanced::_reference_oversampling_level(double p_oversampling)
{
	uint32_t oversampling = CLAMP(p_oversampling, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	OversamplingLevel* ol = oversampling_levels.getptr(oversampling);
	if (ol) {
		ol->refcount++;
	}
	else {
		OversamplingLevel new_ol;
		oversampling_levels.insert(oversampling, new_ol);
	}
}

void TextServerAdvanced::_unreference_oversampling_level(double p_oversampling)
{
	uint32_t oversampling = CLAMP(p_oversampling, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	OversamplingLevel* ol = oversampling_levels.getptr(oversampling);
	if (ol) {
		ol->refcount--;
		if (ol->refcount == 0) {
			for (FontForSizeAdvanced* fd : ol->fonts) {
				fd->owner->cache.erase(fd->size);
				memdelete(fd);
			}
			ol->fonts.clear();
			oversampling_levels.erase(oversampling);
		}
	}
}

_FORCE_INLINE_ bool TextServerAdvanced::_font_validate(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	return _ensure_cache_for_size(fd, size, ffsd, true);
}

bool TextServerAdvanced::_font_is_color(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), false);
#ifdef MODULE_FREETYPE_ENABLED
	return fd->face && FT_HAS_COLOR(fd->face);
#else
	return false;
#endif
}

hb_font_t* TextServerAdvanced::_font_get_hb_handle(
	const RID& p_font_rid, int64_t p_size, bool& r_is_color) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, nullptr);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), nullptr);
#ifdef MODULE_FREETYPE_ENABLED
	r_is_color = fd->face && FT_HAS_COLOR(fd->face);
#else
	r_is_color = false;
#endif

	return ffsd->hb_handle;
}

RID TextServerAdvanced::_create_font()
{
	_THREAD_SAFE_METHOD_

	FontAdvanced* fd = memnew(FontAdvanced);

	return font_owner.make_rid(fd);
}

RID TextServerAdvanced::_create_font_linked_variation(const RID& p_font_rid)
{
	_THREAD_SAFE_METHOD_

	RID rid = p_font_rid;
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(rid);
	if (unlikely(fdv)) {
		rid = fdv->base_font;
	}
	ERR_FAIL_COND_V(!font_owner.owns(rid), RID());

	FontAdvancedLinkedVariation* new_fdv = memnew(FontAdvancedLinkedVariation);
	new_fdv->base_font = rid;

	return font_var_owner.make_rid(new_fdv);
}

void TextServerAdvanced::_font_set_data(const RID& p_font_rid, const PackedByteArray& p_data)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->data = p_data;
	fd->data_ptr = fd->data.ptr();
	fd->data_size = fd->data.size();
}

void TextServerAdvanced::_font_set_data_ptr(
	const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->data.resize(0);
	fd->data_ptr = p_data_ptr;
	fd->data_size = p_data_size;
}

void TextServerAdvanced::_font_set_face_index(const RID& p_font_rid, int64_t p_face_index)
{
	ERR_FAIL_COND(p_face_index < 0);
	ERR_FAIL_COND(p_face_index >= 0x7FFF);

	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->face_index != p_face_index) {
		fd->face_index = p_face_index;
	}
}

int64_t TextServerAdvanced::_font_get_face_index(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->face_index;
}

int64_t TextServerAdvanced::_font_get_face_count(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	int face_count = 0;

	if (fd->data_ptr && (fd->data_size > 0)) {
		// Init dynamic font.
#ifdef MODULE_FREETYPE_ENABLED
		int error = 0;
		if (!ft_library) {
			error = FT_Init_FreeType(&ft_library);
			ERR_FAIL_COND_V_MSG(error != 0, false,
				"FreeType: Error initializing library: '" + String(FT_Error_String(error)) + "'.");
#ifdef MODULE_SVG_ENABLED
			FT_Property_Set(ft_library, "ot-svg", "svg-hooks", get_tvg_svg_in_ot_hooks());
#endif
		}

		FT_StreamRec stream;
		memset(&stream, 0, sizeof(FT_StreamRec));
		stream.base = (unsigned char*)fd->data_ptr;
		stream.size = fd->data_size;
		stream.pos = 0;

		FT_Open_Args fargs;
		memset(&fargs, 0, sizeof(FT_Open_Args));
		fargs.memory_base = (unsigned char*)fd->data_ptr;
		fargs.memory_size = fd->data_size;
		fargs.flags = FT_OPEN_MEMORY;
		fargs.stream = &stream;

		MutexLock ftlock(ft_mutex);

		FT_Face tmp_face = nullptr;
		error = FT_Open_Face(ft_library, &fargs, -1, &tmp_face);
		if (error == 0) {
			face_count = tmp_face->num_faces;
			FT_Done_Face(tmp_face);
		}
#endif
	}

	return face_count;
}

void TextServerAdvanced::_font_set_style(const RID& p_font_rid, uint32_t p_style)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->style_flags = p_style;
}

uint32_t TextServerAdvanced::_font_get_style(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);
	return fd->style_flags;
}

void TextServerAdvanced::_font_set_style_name(const RID& p_font_rid, const String& p_name)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->style_name = p_name;
}

String TextServerAdvanced::_font_get_style_name(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), String());
	return fd->style_name;
}

void TextServerAdvanced::_font_set_weight(const RID& p_font_rid, int64_t p_weight)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->weight = CLAMP(p_weight, 100, 999);
}

int64_t TextServerAdvanced::_font_get_weight(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 400);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 400);
	return fd->weight;
}

void TextServerAdvanced::_font_set_stretch(const RID& p_font_rid, int64_t p_stretch)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->stretch = CLAMP(p_stretch, 50, 200);
}

int64_t TextServerAdvanced::_font_get_stretch(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 100);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 100);
	return fd->stretch;
}

void TextServerAdvanced::_font_set_name(const RID& p_font_rid, const String& p_name)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->font_name = p_name;
}

String TextServerAdvanced::_font_get_name(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), String());
	return fd->font_name;
}

void TextServerAdvanced::_font_set_antialiasing(
	const RID& p_font_rid, TextServer::FontAntialiasing p_antialiasing)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->antialiasing != p_antialiasing) {
		fd->antialiasing = p_antialiasing;
	}
}

TextServer::FontAntialiasing TextServerAdvanced::_font_get_antialiasing(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, TextServer::FONT_ANTIALIASING_NONE);

	MutexLock lock(fd->mutex);
	return fd->antialiasing;
}

void TextServerAdvanced::_font_set_disable_embedded_bitmaps(
	const RID& p_font_rid, bool p_disable_embedded_bitmaps)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		fd->disable_embedded_bitmaps = p_disable_embedded_bitmaps;
	}
}

bool TextServerAdvanced::_font_get_disable_embedded_bitmaps(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->disable_embedded_bitmaps;
}

void TextServerAdvanced::_font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->mipmaps != p_generate_mipmaps) {
		for (KeyValue<Vector2i, FontForSizeAdvanced*>& E : fd->cache) {
			for (int i = 0; i < E.value->textures.size(); i++) {
				E.value->textures.write[i].dirty = true;
				E.value->textures.write[i].texture = Ref<ImageTexture>();
			}
		}
		fd->mipmaps = p_generate_mipmaps;
	}
}

bool TextServerAdvanced::_font_get_generate_mipmaps(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->mipmaps;
}

void TextServerAdvanced::_font_set_multichannel_signed_distance_field(
	const RID& p_font_rid, bool p_msdf)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf != p_msdf) {
		fd->msdf = p_msdf;
	}
}

bool TextServerAdvanced::_font_is_multichannel_signed_distance_field(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->msdf;
}

void TextServerAdvanced::_font_set_msdf_pixel_range(
	const RID& p_font_rid, int64_t p_msdf_pixel_range)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf_range != p_msdf_pixel_range) {
		fd->msdf_range = p_msdf_pixel_range;
	}
}

int64_t TextServerAdvanced::_font_get_msdf_pixel_range(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->msdf_range;
}

void TextServerAdvanced::_font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf_source_size != p_msdf_size) {
		fd->msdf_source_size = p_msdf_size;
	}
}

int64_t TextServerAdvanced::_font_get_msdf_size(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->msdf_source_size;
}

void TextServerAdvanced::_font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->fixed_size = p_fixed_size;
}

int64_t TextServerAdvanced::_font_get_fixed_size(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->fixed_size;
}

void TextServerAdvanced::_font_set_fixed_size_scale_mode(
	const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->fixed_size_scale_mode = p_fixed_size_scale_mode;
}

TextServer::FixedSizeScaleMode TextServerAdvanced::_font_get_fixed_size_scale_mode(
	const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, FIXED_SIZE_SCALE_DISABLE);

	MutexLock lock(fd->mutex);
	return fd->fixed_size_scale_mode;
}

void TextServerAdvanced::_font_set_allow_system_fallback(
	const RID& p_font_rid, bool p_allow_system_fallback)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->allow_system_fallback = p_allow_system_fallback;
}

bool TextServerAdvanced::_font_is_allow_system_fallback(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->allow_system_fallback;
}

void TextServerAdvanced::_font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->force_autohinter != p_force_autohinter) {
		fd->force_autohinter = p_force_autohinter;
	}
}

bool TextServerAdvanced::_font_is_force_autohinter(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->force_autohinter;
}

void TextServerAdvanced::_font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->modulate_color_glyphs != p_modulate) {
		fd->modulate_color_glyphs = p_modulate;
	}
}

bool TextServerAdvanced::_font_is_modulate_color_glyphs(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->modulate_color_glyphs;
}

int64_t TextServerAdvanced::_font_get_palette_count(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_names.size();
#else
	return 0;
#endif
}

String TextServerAdvanced::_font_get_palette_name(const RID& p_font_rid, int64_t p_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

#if HB_VERSION_ATLEAST(13, 0, 0)
	ERR_FAIL_INDEX_V(p_index, fd->palette_names.size(), String());
	return fd->palette_names[p_index];
#else
	return String();
#endif
}

Vector<Color> TextServerAdvanced::_font_get_palette_colors(
	const RID& p_font_rid, int64_t p_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector<Color>());

#if HB_VERSION_ATLEAST(13, 0, 0)
	ERR_FAIL_INDEX_V(p_index, fd->palette_names.size(), Vector<Color>());
	return fd->palette_colors[p_index];
#else
	return Vector<Color>();
#endif
}

void TextServerAdvanced::_font_set_palette_custom_colors(
	const RID& p_font_rid, const Vector<Color>& p_colors)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

#if HB_VERSION_ATLEAST(13, 0, 0)
	if (fd->palette_custom_colors != p_colors) {
		fd->palette_custom_colors = p_colors;
		fd->palette_custom_colors_hb.resize_uninitialized(p_colors.size());
		for (int col = 0; col < fd->palette_custom_colors.size(); col++) {
			const Color& c = fd->palette_custom_colors[col];
			fd->palette_custom_colors_hb.write[col] =
				HB_COLOR(c.get_b8(), c.get_g8(), c.get_r8(), c.get_a8());
		}
	}
#endif
}

Vector<Color> TextServerAdvanced::_font_get_palette_custom_colors(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector<Color>());

#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_custom_colors;
#else
	return Vector<Color>();
#endif
}

int64_t TextServerAdvanced::_font_get_used_palette(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);
#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_index;
#else
	return 0;
#endif
}

void TextServerAdvanced::_font_set_used_palette(const RID& p_font_rid, int64_t p_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
#if HB_VERSION_ATLEAST(13, 0, 0)
	if (fd->palette_index != p_index) {
		fd->palette_index = p_index;
	}
#endif
}

void TextServerAdvanced::_font_set_hinting(const RID& p_font_rid, TextServer::Hinting p_hinting)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->hinting != p_hinting) {
		fd->hinting = p_hinting;
	}
}

TextServer::Hinting TextServerAdvanced::_font_get_hinting(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, HINTING_NONE);

	MutexLock lock(fd->mutex);
	return fd->hinting;
}

void TextServerAdvanced::_font_set_subpixel_positioning(
	const RID& p_font_rid, TextServer::SubpixelPositioning p_subpixel)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->subpixel_positioning = p_subpixel;
}

TextServer::SubpixelPositioning TextServerAdvanced::_font_get_subpixel_positioning(
	const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, SUBPIXEL_POSITIONING_DISABLED);

	MutexLock lock(fd->mutex);
	return fd->subpixel_positioning;
}

void TextServerAdvanced::_font_set_keep_rounding_remainders(
	const RID& p_font_rid, bool p_keep_rounding_remainders)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->keep_rounding_remainders = p_keep_rounding_remainders;
}

bool TextServerAdvanced::_font_get_keep_rounding_remainders(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->keep_rounding_remainders;
}

void TextServerAdvanced::_font_set_embolden(const RID& p_font_rid, double p_strength)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->embolden != p_strength) {
		fd->embolden = p_strength;
	}
}

double TextServerAdvanced::_font_get_embolden(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	return fd->embolden;
}

void TextServerAdvanced::_font_set_spacing(
	const RID& p_font_rid, SpacingType p_spacing, int64_t p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, 4);
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		if (fdv->extra_spacing[p_spacing] != p_value) {
			fdv->extra_spacing[p_spacing] = p_value;
		}
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL(fd);

		MutexLock lock(fd->mutex);
		if (fd->extra_spacing[p_spacing] != p_value) {
			fd->extra_spacing[p_spacing] = p_value;
		}
	}
}

int64_t TextServerAdvanced::_font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, 4, 0);
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		return fdv->extra_spacing[p_spacing];
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL_V(fd, 0);

		MutexLock lock(fd->mutex);
		return fd->extra_spacing[p_spacing];
	}
}

void TextServerAdvanced::_font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset)
{
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		if (fdv->baseline_offset != p_baseline_offset) {
			fdv->baseline_offset = p_baseline_offset;
		}
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL(fd);

		MutexLock lock(fd->mutex);
		if (fd->baseline_offset != p_baseline_offset) {
			fd->baseline_offset = p_baseline_offset;
		}
	}
}

double TextServerAdvanced::_font_get_baseline_offset(const RID& p_font_rid) const
{
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		return fdv->baseline_offset;
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL_V(fd, 0.0);

		MutexLock lock(fd->mutex);
		return fd->baseline_offset;
	}
}

void TextServerAdvanced::font_set_transform(const RID& p_font_rid, const Transform2D& p_transform)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->transform != p_transform) {
		fd->transform = p_transform;
	}
}

Transform2D TextServerAdvanced::_font_get_transform(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Transform2D());

	MutexLock lock(fd->mutex);
	return fd->transform;
}

double TextServerAdvanced::_font_get_oversampling(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, -1.0);

	MutexLock lock(fd->mutex);
	return fd->oversampling_override;
}

void TextServerAdvanced::_font_set_oversampling(const RID& p_font_rid, double p_oversampling)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->oversampling_override != p_oversampling) {
		fd->oversampling_override = p_oversampling;
	}
}

void TextServerAdvanced::_font_clear_size_cache(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	MutexLock ftlock(ft_mutex);
	for (const KeyValue<Vector2i, FontForSizeAdvanced*>& E : fd->cache) {
		if (E.value->viewport_oversampling != 0) {
			OversamplingLevel* ol = oversampling_levels.getptr(E.value->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(E.value);
			}
		}
		memdelete(E.value);
	}
	fd->cache.clear();
}

void TextServerAdvanced::_font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	MutexLock ftlock(ft_mutex);
	Vector2i size = Vector2i(p_size.x * 64, p_size.y);
	if (fd->cache.has(size)) {
		if (fd->cache[size]->viewport_oversampling != 0) {
			OversamplingLevel* ol =
				oversampling_levels.getptr(fd->cache[size]->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(fd->cache[size]);
			}
		}
		memdelete(fd->cache[size]);
		fd->cache.erase(size);
	}
}

void TextServerAdvanced::_font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->ascent = p_ascent;
}

double TextServerAdvanced::_font_get_ascent(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->ascent * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->ascent * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->ascent * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->ascent;
	}
}

void TextServerAdvanced::_font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->descent = p_descent;
}

double TextServerAdvanced::_font_get_descent(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->descent * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->descent * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->descent * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->descent;
	}
}

void TextServerAdvanced::_font_set_underline_position(
	const RID& p_font_rid, int64_t p_size, double p_underline_position)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->underline_position = p_underline_position;
}

double TextServerAdvanced::_font_get_underline_position(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->underline_position * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->underline_position * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->underline_position * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->underline_position;
	}
}

void TextServerAdvanced::_font_set_underline_thickness(
	const RID& p_font_rid, int64_t p_size, double p_underline_thickness)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->underline_thickness = p_underline_thickness;
}

double TextServerAdvanced::_font_get_underline_thickness(
	const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->underline_thickness * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->underline_thickness * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->underline_thickness * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->underline_thickness;
	}
}

void TextServerAdvanced::_font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		return; // Do not override scale for dynamic fonts, it's calculated automatically.
	}
#endif
	ffsd->scale = p_scale;
}

double TextServerAdvanced::_font_get_scale(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->scale * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->scale * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->scale * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->scale;
	}
}

int64_t TextServerAdvanced::_font_get_texture_count(
	const RID& p_font_rid, const Vector2i& p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

	return ffsd->textures.size();
}

void TextServerAdvanced::_font_clear_textures(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->textures.clear();
}

void TextServerAdvanced::_font_remove_texture(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_INDEX(p_texture_index, ffsd->textures.size());

	ffsd->textures.remove_at(p_texture_index);
}

void TextServerAdvanced::_font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
	int64_t p_texture_index, const Ref<Image>& p_image)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	ERR_FAIL_COND(p_image.is_null());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_COND(p_texture_index < 0);
	if (p_texture_index >= ffsd->textures.size()) {
		ffsd->textures.resize(p_texture_index + 1);
	}

	ShelfPackTexture& tex = ffsd->textures.write[p_texture_index];

	tex.image = p_image;
	tex.texture_w = p_image->get_width();
	tex.texture_h = p_image->get_height();

	Ref<Image> img = p_image;
	tex.texture = ImageTexture::create_from_image(img);
	tex.dirty = false;
}

Ref<Image> TextServerAdvanced::_font_get_texture_image(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Ref<Image>());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Ref<Image>());
	ERR_FAIL_INDEX_V(p_texture_index, ffsd->textures.size(), Ref<Image>());

	const ShelfPackTexture& tex = ffsd->textures[p_texture_index];
	return tex.image;
}

void TextServerAdvanced::_font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
	int64_t p_texture_index, const PackedInt32Array& p_offsets)
{
	ERR_FAIL_COND(p_offsets.size() % 4 != 0);
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_COND(p_texture_index < 0);
	if (p_texture_index >= ffsd->textures.size()) {
		ffsd->textures.resize(p_texture_index + 1);
	}

	ShelfPackTexture& tex = ffsd->textures.write[p_texture_index];
	tex.shelves.clear();
	for (int32_t i = 0; i < p_offsets.size(); i += 4) {
		tex.shelves.push_back(
			Shelf(p_offsets[i], p_offsets[i + 1], p_offsets[i + 2], p_offsets[i + 3]));
	}
}

PackedInt32Array TextServerAdvanced::_font_get_texture_offsets(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), PackedInt32Array());
	ERR_FAIL_INDEX_V(p_texture_index, ffsd->textures.size(), PackedInt32Array());

	const ShelfPackTexture& tex = ffsd->textures[p_texture_index];
	PackedInt32Array ret;
	ret.resize(tex.shelves.size() * 4);

	int32_t* wr = ret.ptrw();
	int32_t i = 0;
	for (const Shelf& E : tex.shelves) {
		wr[i * 4] = E.x;
		wr[i * 4 + 1] = E.y;
		wr[i * 4 + 2] = E.w;
		wr[i * 4 + 3] = E.h;
		i++;
	}
	return ret;
}

PackedInt32Array TextServerAdvanced::_font_get_glyph_list(
	const RID& p_font_rid, const Vector2i& p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), PackedInt32Array());

	PackedInt32Array ret;
	const HashMap<int32_t, FontGlyph>& gl = ffsd->glyph_map;
	for (const KeyValue<int32_t, FontGlyph>& E : gl) {
		ret.push_back(E.key);
	}
	return ret;
}

void TextServerAdvanced::_font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	ffsd->glyph_map.clear();
}

void TextServerAdvanced::_font_remove_glyph(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	ffsd->glyph_map.erase(p_glyph);
}

double TextServerAdvanced::_get_extra_advance(RID p_font_rid, int p_font_size) const
{
	const FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_font_size);

	if (fd->embolden != 0.0) {
		return fd->embolden * double(size.x) / 4096.0;
	}
	else {
		return 0.0;
	}
}

Vector2 TextServerAdvanced::_font_get_glyph_advance(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	Vector2 ea;
	if (fd->embolden != 0.0) {
		ea.x = fd->embolden * double(size.x) / 4096.0;
	}

	double scale = _font_get_scale(p_font_rid, p_size);
	if (fd->msdf) {
		return (fgl.advance + ea) * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return (fgl.advance + ea) * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return (fgl.advance + ea) * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else if ((scale == 1.0) && ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_DISABLED) ||
									 (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
										 size.x > SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64))) {
		return (fgl.advance + ea).round();
	}
	else {
		return fgl.advance + ea;
	}
}

void TextServerAdvanced::_font_set_glyph_advance(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.advance = p_advance;
	fgl.found = true;
}

Vector2 TextServerAdvanced::_font_get_glyph_offset(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	if (fd->msdf) {
		return fgl.rect.position * (double)p_size.x / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size.x * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return fgl.rect.position * (double)p_size.x / (double)fd->fixed_size;
		}
		else {
			return fgl.rect.position * Math::round((double)p_size.x / (double)fd->fixed_size);
		}
	}
	else {
		return fgl.rect.position;
	}
}

void TextServerAdvanced::_font_set_glyph_offset(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.rect.position = p_offset;
	fgl.found = true;
}

Vector2 TextServerAdvanced::_font_get_glyph_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	if (fd->msdf) {
		return fgl.rect.size * (double)p_size.x / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size.x * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return fgl.rect.size * (double)p_size.x / (double)fd->fixed_size;
		}
		else {
			return fgl.rect.size * Math::round((double)p_size.x / (double)fd->fixed_size);
		}
	}
	else {
		return fgl.rect.size;
	}
}

void TextServerAdvanced::_font_set_glyph_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.rect.size = p_gl_size;
	fgl.found = true;
}

Rect2 TextServerAdvanced::_font_get_glyph_uv_rect(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Rect2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Rect2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Rect2(); // Invalid or non graphicl glyph, do not display errors.
	}

	return fgl.uv_rect;
}

void TextServerAdvanced::_font_set_glyph_uv_rect(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.uv_rect = p_uv_rect;
	fgl.found = true;
}

int64_t TextServerAdvanced::_font_get_glyph_texture_idx(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, -1);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), -1);

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return -1; // Invalid or non graphicl glyph, do not display errors.
	}

	return fgl.texture_idx;
}

void TextServerAdvanced::_font_set_glyph_texture_idx(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.texture_idx = p_texture_idx;
	fgl.found = true;
}

RID TextServerAdvanced::_font_get_glyph_texture_rid(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, RID());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), RID());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return RID(); // Invalid or non graphicl glyph, do not display errors.
	}

	ERR_FAIL_COND_V(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size(), RID());

	if (fgl.texture_idx != -1) {
		if (ffsd->textures[fgl.texture_idx].dirty) {
			ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
			Ref<Image> img = tex.image;
			if (fgl.fix_edge) {
				// Same as the "fix alpha border" process option when importing SVGs
				img->fix_alpha_edges();
			}
			if (tex.texture.is_null()) {
				tex.texture = ImageTexture::create_from_image(img);
			}
			else {
				tex.texture->update(img);
			}
			tex.dirty = false;
		}
		return ffsd->textures[fgl.texture_idx].texture->get_rid();
	}

	return RID();
}

Size2 TextServerAdvanced::_font_get_glyph_texture_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Size2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Size2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Size2(); // Invalid or non graphicl glyph, do not display errors.
	}

	ERR_FAIL_COND_V(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size(), Size2());

	if (fgl.texture_idx != -1) {
		if (ffsd->textures[fgl.texture_idx].dirty) {
			ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
			Ref<Image> img = tex.image;
			if (fgl.fix_edge) {
				// Same as the "fix alpha border" process option when importing SVGs
				img->fix_alpha_edges();
			}
			if (tex.texture.is_null()) {
				tex.texture = ImageTexture::create_from_image(img);
			}
			else {
				tex.texture->update(img);
			}
			tex.dirty = false;
		}
		return ffsd->textures[fgl.texture_idx].texture->get_size();
	}

	return Size2();
}

void TextServerAdvanced::_font_clear_kerning_map(const RID& p_font_rid, int64_t p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map.clear();
}

void TextServerAdvanced::_font_remove_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map.erase(p_glyph_pair);
}

void TextServerAdvanced::_font_set_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map[p_glyph_pair] = p_kerning;
}

Vector2 TextServerAdvanced::_font_get_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	const HashMap<Vector2i, Vector2>& kern = ffsd->kerning_map;

	if (kern.has(p_glyph_pair)) {
		if (fd->msdf) {
			return kern[p_glyph_pair] * (double)p_size / (double)fd->msdf_source_size;
		}
		else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
				   size.x != p_size * 64) {
			if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
				return kern[p_glyph_pair] * (double)p_size / (double)fd->fixed_size;
			}
			else {
				return kern[p_glyph_pair] * Math::round((double)p_size / (double)fd->fixed_size);
			}
		}
		else {
			return kern[p_glyph_pair];
		}
	}
	else {
#ifdef MODULE_FREETYPE_ENABLED
		if (fd->face) {
			FT_Vector delta;
			FT_Get_Kerning(fd->face, p_glyph_pair.x, p_glyph_pair.y, FT_KERNING_DEFAULT, &delta);
			if (fd->msdf) {
				return Vector2(delta.x, delta.y) * (double)p_size / (double)fd->msdf_source_size;
			}
			else if (fd->fixed_size > 0 &&
					   fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
					   size.x != p_size * 64) {
				if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
					return Vector2(delta.x, delta.y) * (double)p_size / (double)fd->fixed_size;
				}
				else {
					return Vector2(delta.x, delta.y) *
						   Math::round((double)p_size / (double)fd->fixed_size);
				}
			}
			else {
				return Vector2(delta.x, delta.y);
			}
		}
#endif
	}
	return Vector2();
}

int64_t TextServerAdvanced::_font_get_glyph_index(
	const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);
	ERR_FAIL_COND_V_MSG((p_char >= 0xd800 && p_char <= 0xdfff) || (p_char > 0x10ffff), 0,
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_char, 16) + ".");
	ERR_FAIL_COND_V_MSG((p_variation_selector >= 0xd800 && p_variation_selector <= 0xdfff) ||
							(p_variation_selector > 0x10ffff),
		0,
		"Unicode parsing error: Invalid unicode codepoint " +
			String::num_int64(p_variation_selector, 16) + ".");

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		if (p_variation_selector) {
			return FT_Face_GetCharVariantIndex(fd->face, p_char, p_variation_selector);
		}
		else {
			return FT_Get_Char_Index(fd->face, p_char);
		}
	}
	else {
		return (int64_t)p_char;
	}
#else
	return (int64_t)p_char;
#endif
}

int64_t TextServerAdvanced::_font_get_char_from_glyph_index(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

#ifdef MODULE_FREETYPE_ENABLED
	if (ffsd->inv_glyph_map.is_empty()) {
		FT_Face face = fd->face;
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(face, &gindex);
		while (gindex != 0) {
			if (charcode != 0) {
				ffsd->inv_glyph_map[gindex] = charcode;
			}
			charcode = FT_Get_Next_Char(face, charcode, &gindex);
		}
	}

	if (ffsd->inv_glyph_map.has(p_glyph_index)) {
		return ffsd->inv_glyph_map[p_glyph_index];
	}
	else {
		return 0;
	}
#else
	return p_glyph_index;
#endif
}

bool TextServerAdvanced::_font_has_char(const RID& p_font_rid, int64_t p_char) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_COND_V_MSG((p_char >= 0xd800 && p_char <= 0xdfff) || (p_char > 0x10ffff), false,
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_char, 16) + ".");
	if (!fd) {
		return false;
	}

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* ffsd = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(
				fd, fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), ffsd),
			false);
	}
	else {
		ffsd = fd->cache.begin()->value;
	}

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		return FT_Get_Char_Index(fd->face, p_char) != 0;
	}
#endif
	return ffsd->glyph_map.has((int32_t)p_char);
}

String TextServerAdvanced::_font_get_supported_chars(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* ffsd = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(
				fd, fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), ffsd),
			String());
	}
	else {
		ffsd = fd->cache.begin()->value;
	}

	String chars;
#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(fd->face, &gindex);
		while (gindex != 0) {
			if (charcode != 0) {
				chars = chars + String::chr(charcode);
			}
			charcode = FT_Get_Next_Char(fd->face, charcode, &gindex);
		}
		return chars;
	}
#endif
	const HashMap<int32_t, FontGlyph>& gl = ffsd->glyph_map;
	for (const KeyValue<int32_t, FontGlyph>& E : gl) {
		chars = chars + String::chr(E.key);
	}
	return chars;
}

PackedInt32Array TextServerAdvanced::_font_get_supported_glyphs(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* at_size = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(fd,
				fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), at_size),
			PackedInt32Array());
	}
	else {
		at_size = fd->cache.begin()->value;
	}

	PackedInt32Array glyphs;
#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(fd->face, &gindex);
		while (gindex != 0) {
			glyphs.push_back(gindex);
			charcode = FT_Get_Next_Char(fd->face, charcode, &gindex);
		}
		return glyphs;
	}
#endif
	if (at_size) {
		const HashMap<int32_t, FontGlyph>& gl = at_size->glyph_map;
		for (const KeyValue<int32_t, FontGlyph>& E : gl) {
			glyphs.push_back(E.key);
		}
	}
	return glyphs;
}

void TextServerAdvanced::_font_render_range(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_start, int64_t p_end)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	ERR_FAIL_COND_MSG((p_start >= 0xd800 && p_start <= 0xdfff) || (p_start > 0x10ffff),
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_start, 16) + ".");
	ERR_FAIL_COND_MSG((p_end >= 0xd800 && p_end <= 0xdfff) || (p_end > 0x10ffff),
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_end, 16) + ".");

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	for (int64_t i = p_start; i <= p_end; i++) {
#ifdef MODULE_FREETYPE_ENABLED
		int32_t idx = FT_Get_Char_Index(fd->face, i);
		if (fd->face) {
			FontGlyph fgl;
			if (fd->msdf) {
				_ensure_glyph(fd, size, (int32_t)idx, fgl);
			}
			else {
				for (int aa = 0; aa < ((fd->antialiasing == FONT_ANTIALIASING_LCD)
											  ? FONT_LCD_SUBPIXEL_LAYOUT_MAX
											  : 1);
					 aa++) {
					if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
						(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
						_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (2 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (3 << 27) | (aa << 24), fgl);
					}
					else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
							   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
								   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
						_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
					}
					else {
						_ensure_glyph(fd, size, (int32_t)idx | (aa << 24), fgl);
					}
				}
			}
		}
#endif
	}
}

void TextServerAdvanced::_font_render_glyph(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
#ifdef MODULE_FREETYPE_ENABLED
	int32_t idx = p_index & 0xffffff; // Remove subpixel shifts.
	if (fd->face) {
		FontGlyph fgl;
		if (fd->msdf) {
			_ensure_glyph(fd, size, (int32_t)idx, fgl);
		}
		else {
			for (int aa = 0;
				 aa <
				 ((fd->antialiasing == FONT_ANTIALIASING_LCD) ? FONT_LCD_SUBPIXEL_LAYOUT_MAX : 1);
				 aa++) {
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (2 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (3 << 27) | (aa << 24), fgl);
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
				}
				else {
					_ensure_glyph(fd, size, (int32_t)idx | (aa << 24), fgl);
				}
			}
		}
	}
#endif
}

void TextServerAdvanced::_font_draw_glyph(const RID& p_font_rid, const RID& p_canvas,
	int64_t p_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
	float p_oversampling) const
{
	if (p_index == 0) {
		return; // Non visual character, skip.
	}
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);

	// Oversampling.
	bool viewport_oversampling = false;
	float oversampling_factor = p_oversampling;
	if (p_oversampling <= 0.0) {
		if (fd->oversampling_override > 0.0) {
			oversampling_factor = fd->oversampling_override;
		}
		else if (vp_oversampling > 0.0) {
			oversampling_factor = vp_oversampling;
			viewport_oversampling = true;
		}
		else {
			oversampling_factor = 1.0;
		}
	}
	bool skip_oversampling = fd->msdf || fd->fixed_size > 0;
	if (skip_oversampling) {
		oversampling_factor = 1.0;
	}
	else {
		uint64_t oversampling_level = CLAMP(oversampling_factor, 0.1, 100.0) * 64;
		oversampling_factor = double(oversampling_level) / 64.0;
	}

	Vector2i size;
	if (skip_oversampling) {
		size = _get_size(fd, p_size);
	}
	else {
		size = Vector2i(p_size * 64 * oversampling_factor, 0);
	}

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(
		fd, size, ffsd, false, viewport_oversampling ? 64 * oversampling_factor : 0));

	int32_t index = p_index & 0xffffff; // Remove subpixel shifts.
	bool lcd_aa = false;

#ifdef MODULE_FREETYPE_ENABLED
	if (!fd->msdf && fd->face) {
		// LCD layout, bits 24, 25, 26
		if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
			TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
			if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
				lcd_aa = true;
				index = index | (layout << 24);
			}
		}
		// Subpixel X-shift, bits 27, 28
		if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
			(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
				size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
			int xshift =
				(int)(Math::floor(4 * (p_pos.x + 0.125)) - 4 * Math::floor(p_pos.x + 0.125));
			index = index | (xshift << 27);
		}
		else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
				   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
			int xshift = (int)(Math::floor(2 * (p_pos.x + 0.25)) - 2 * Math::floor(p_pos.x + 0.25));
			index = index | (xshift << 27);
		}
	}
#endif

	FontGlyph fgl;
	if (!_ensure_glyph(
			fd, size, index, fgl, viewport_oversampling ? 64 * oversampling_factor : 0)) {
		return; // Invalid or non-graphical glyph, do not display errors, nothing to draw.
	}

	if (fgl.found) {
		ERR_FAIL_COND(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size());

		if (fgl.texture_idx != -1) {
			Color modulate = p_color;
#ifdef MODULE_FREETYPE_ENABLED
			if (!fd->modulate_color_glyphs && fd->face &&
				ffsd->textures[fgl.texture_idx].image.is_valid() &&
				(ffsd->textures[fgl.texture_idx].image->get_format() == Image::FORMAT_RGBA8) &&
				!lcd_aa && !fd->msdf) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}
#endif
			if (ffsd->textures[fgl.texture_idx].dirty) {
				ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
				Ref<Image> img = tex.image;
				if (fgl.fix_edge) {
					// Same as the "fix alpha border" process option when importing SVGs
					img->fix_alpha_edges();
				}
				if (tex.texture.is_null()) {
					tex.texture = ImageTexture::create_from_image(img);
				}
				else {
					tex.texture->update(img);
				}
				tex.dirty = false;
			}
			if (fd->msdf) {
				Point2 cpos = p_pos;
				cpos += fgl.rect.position * (double)p_size / (double)fd->msdf_source_size;
				Size2 csize = fgl.rect.size * (double)p_size / (double)fd->msdf_source_size;
				ffsd->textures[fgl.texture_idx].texture->draw_msdf_rect_region(p_canvas,
					Rect2(cpos, csize), fgl.uv_rect, modulate, 0, fd->msdf_range,
					(double)p_size / (double)fd->msdf_source_size);
			}
			else {
				Point2 cpos = p_pos;
				double scale = _font_get_scale(p_font_rid, p_size) / oversampling_factor;
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.125;
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.25;
				}
				if (scale == 1.0) {
					cpos.y = Math::floor(cpos.y);
					cpos.x = Math::floor(cpos.x);
				}
				Vector2 gpos = fgl.rect.position;
				Size2 csize = fgl.rect.size;
				if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE) {
					if (size.x != p_size * 64) {
						if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
							double gl_scale = (double)p_size / (double)fd->fixed_size;
							gpos *= gl_scale;
							csize *= gl_scale;
						}
						else {
							double gl_scale = Math::round((double)p_size / (double)fd->fixed_size);
							gpos *= gl_scale;
							csize *= gl_scale;
						}
					}
				}
				else {
					gpos /= oversampling_factor;
					csize /= oversampling_factor;
				}
				cpos += gpos;
				if (lcd_aa) {
					ffsd->textures[fgl.texture_idx].texture->draw_lcd_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate);
				}
				else {
					ffsd->textures[fgl.texture_idx].texture->draw_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate, false, false);
				}
			}
		}
	}
}

void TextServerAdvanced::_font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas,
	int64_t p_size, int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
	const Color& p_color, float p_oversampling) const
{
	if (p_index == 0) {
		return; // Non visual character, skip.
	}
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);

	// Oversampling.
	bool viewport_oversampling = false;
	float oversampling_factor = p_oversampling;
	if (p_oversampling <= 0.0) {
		if (fd->oversampling_override > 0.0) {
			oversampling_factor = fd->oversampling_override;
		}
		else if (vp_oversampling > 0.0) {
			oversampling_factor = vp_oversampling;
			viewport_oversampling = true;
		}
		else {
			oversampling_factor = 1.0;
		}
	}
	bool skip_oversampling = fd->msdf || fd->fixed_size > 0;
	if (skip_oversampling) {
		oversampling_factor = 1.0;
	}
	else {
		uint64_t oversampling_level = CLAMP(oversampling_factor, 0.1, 100.0) * 64;
		oversampling_factor = double(oversampling_level) / 64.0;
	}

	Vector2i size;
	if (skip_oversampling) {
		size = _get_size_outline(fd, Vector2i(p_size, p_outline_size));
	}
	else {
		size = Vector2i(p_size * 64 * oversampling_factor, p_outline_size * oversampling_factor);
	}

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(
		fd, size, ffsd, false, viewport_oversampling ? 64 * oversampling_factor : 0));

	int32_t index = p_index & 0xffffff; // Remove subpixel shifts.
	bool lcd_aa = false;

#ifdef MODULE_FREETYPE_ENABLED
	if (!fd->msdf && fd->face) {
		// LCD layout, bits 24, 25, 26
		if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
			TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
			if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
				lcd_aa = true;
				index = index | (layout << 24);
			}
		}
		// Subpixel X-shift, bits 27, 28
		if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
			(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
				size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
			int xshift =
				(int)(Math::floor(4 * (p_pos.x + 0.125)) - 4 * Math::floor(p_pos.x + 0.125));
			index = index | (xshift << 27);
		}
		else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
				   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
			int xshift = (int)(Math::floor(2 * (p_pos.x + 0.25)) - 2 * Math::floor(p_pos.x + 0.25));
			index = index | (xshift << 27);
		}
	}
#endif

	FontGlyph fgl;
	if (!_ensure_glyph(
			fd, size, index, fgl, viewport_oversampling ? 64 * oversampling_factor : 0)) {
		return; // Invalid or non-graphical glyph, do not display errors, nothing to draw.
	}

	if (fgl.found) {
		ERR_FAIL_COND(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size());

		if (fgl.texture_idx != -1) {
			Color modulate = p_color;
#ifdef MODULE_FREETYPE_ENABLED
			if (fd->face && fd->cache[size]->textures[fgl.texture_idx].image.is_valid() &&
				(ffsd->textures[fgl.texture_idx].image->get_format() == Image::FORMAT_RGBA8) &&
				!lcd_aa && !fd->msdf) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}
#endif
			if (ffsd->textures[fgl.texture_idx].dirty) {
				ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
				Ref<Image> img = tex.image;
				if (tex.texture.is_null()) {
					tex.texture = ImageTexture::create_from_image(img);
				}
				else {
					tex.texture->update(img);
				}
				tex.dirty = false;
			}
			if (fd->msdf) {
				Point2 cpos = p_pos;
				cpos += fgl.rect.position * (double)p_size / (double)fd->msdf_source_size;
				Size2 csize = fgl.rect.size * (double)p_size / (double)fd->msdf_source_size;
				ffsd->textures[fgl.texture_idx].texture->draw_msdf_rect_region(p_canvas,
					Rect2(cpos, csize), fgl.uv_rect, modulate, p_outline_size, fd->msdf_range,
					(double)p_size / (double)fd->msdf_source_size);
			}
			else {
				Point2 cpos = p_pos;
				double scale = _font_get_scale(p_font_rid, p_size) / oversampling_factor;
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.125;
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.25;
				}
				if (scale == 1.0) {
					cpos.y = Math::floor(cpos.y);
					cpos.x = Math::floor(cpos.x);
				}
				Vector2 gpos = fgl.rect.position;
				Size2 csize = fgl.rect.size;
				if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE) {
					if (size.x != p_size * 64) {
						if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
							double gl_scale = (double)p_size / (double)fd->fixed_size;
							gpos *= gl_scale;
							csize *= gl_scale;
						}
						else {
							double gl_scale = Math::round((double)p_size / (double)fd->fixed_size);
							gpos *= gl_scale;
							csize *= gl_scale;
						}
					}
				}
				else {
					gpos /= oversampling_factor;
					csize /= oversampling_factor;
				}
				cpos += gpos;
				if (lcd_aa) {
					ffsd->textures[fgl.texture_idx].texture->draw_lcd_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate);
				}
				else {
					ffsd->textures[fgl.texture_idx].texture->draw_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate, false, false);
				}
			}
		}
	}
}

bool TextServerAdvanced::_font_is_language_supported(
	const RID& p_font_rid, const String& p_language) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	if (fd->language_support_overrides.has(p_language)) {
		return fd->language_support_overrides[p_language];
	}
	else {
		if (fd->language_support_overrides.has("*")) {
			return fd->language_support_overrides["*"];
		}
		return true;
	}
}

void TextServerAdvanced::_font_set_language_support_override(
	const RID& p_font_rid, const String& p_language, bool p_supported)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->language_support_overrides[p_language] = p_supported;
}

bool TextServerAdvanced::_font_get_language_support_override(
	const RID& p_font_rid, const String& p_language)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->language_support_overrides[p_language];
}

void TextServerAdvanced::_font_remove_language_support_override(
	const RID& p_font_rid, const String& p_language)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->language_support_overrides.erase(p_language);
}

PackedStringArray TextServerAdvanced::_font_get_language_support_overrides(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedStringArray());

	MutexLock lock(fd->mutex);
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : fd->language_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

bool TextServerAdvanced::_font_is_script_supported(
	const RID& p_font_rid, const String& p_script) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	if (fd->script_support_overrides.has(p_script)) {
		return fd->script_support_overrides[p_script];
	}
	else {
		if (fd->script_support_overrides.has("*")) {
			return fd->script_support_overrides["*"];
		}
		Vector2i size = _get_size(fd, 16);
		FontForSizeAdvanced* ffsd = nullptr;
		ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), false);
		char ascii_script[] = {' ', ' ', ' ', ' '};
		for (int i = 0; i < MIN(4, p_script.size()); i++) {
			if (p_script[i] <= 0x7f) {
				ascii_script[i] = p_script[i];
			}
		}
		return fd->supported_scripts.has(hb_tag_from_string(ascii_script, -1));
	}
}

void TextServerAdvanced::_font_set_script_support_override(
	const RID& p_font_rid, const String& p_script, bool p_supported)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->script_support_overrides[p_script] = p_supported;
}

bool TextServerAdvanced::_font_get_script_support_override(
	const RID& p_font_rid, const String& p_script)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->script_support_overrides[p_script];
}

void TextServerAdvanced::_font_remove_script_support_override(
	const RID& p_font_rid, const String& p_script)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->script_support_overrides.erase(p_script);
}

PackedStringArray TextServerAdvanced::_font_get_script_support_overrides(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedStringArray());

	MutexLock lock(fd->mutex);
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : fd->script_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

/*************************************************************************/
/* Shaped text buffer interface                                          */
/*************************************************************************/

int64_t TextServerAdvanced::_convert_pos(
	const String& p_utf32, const Char16String& p_utf16, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_utf32.length() != p_utf16.length()) {
		const UChar* data = p_utf16.get_data();
		for (int i = 0; i < p_pos; i++) {
			if (U16_IS_LEAD(data[i])) {
				limit--;
			}
		}
	}
	return limit;
}

int64_t TextServerAdvanced::_convert_pos(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_sd->text.length() != p_sd->utf16.length()) {
		const UChar* data = p_sd->utf16.get_data();
		for (int i = 0; i < p_pos; i++) {
			if (U16_IS_LEAD(data[i])) {
				limit--;
			}
		}
	}
	return limit;
}

int64_t TextServerAdvanced::_convert_pos_inv(
	const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_sd->text.length() != p_sd->utf16.length()) {
		for (int i = 0; i < p_pos; i++) {
			if (p_sd->text[i] > 0xffff) {
				limit++;
			}
		}
	}
	return limit;
}

void TextServerAdvanced::invalidate(
	TextServerAdvanced::ShapedTextDataAdvanced* p_shaped, bool p_text)
{
	p_shaped->valid.clear();
	p_shaped->sort_valid = false;
	p_shaped->line_breaks_valid = false;
	p_shaped->justification_ops_valid = false;
	p_shaped->text_trimmed = false;
	p_shaped->ascent = 0.0;
	p_shaped->descent = 0.0;
	p_shaped->width = 0.0;
	p_shaped->upos = 0.0;
	p_shaped->uthk = 0.0;
	p_shaped->glyphs.clear();
	p_shaped->glyphs_logical.clear();
	p_shaped->runs.clear();
	p_shaped->runs_dirty = true;
	p_shaped->overrun_trim_data = TrimData();
	p_shaped->utf16 = Char16String();
	for (int i = 0; i < p_shaped->bidi_iter.size(); i++) {
		ubidi_close(p_shaped->bidi_iter[i]);
	}
	p_shaped->bidi_iter.clear();

	if (p_text) {
		if (p_shaped->script_iter != nullptr) {
			memdelete(p_shaped->script_iter);
			p_shaped->script_iter = nullptr;
		}
		p_shaped->break_ops_valid = false;
		p_shaped->chars_valid = false;
		p_shaped->js_ops_valid = false;
	}
}

RID TextServerAdvanced::_create_shaped_text(
	TextServer::Direction p_direction, TextServer::Orientation p_orientation)
{
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_V_MSG(p_direction == DIRECTION_INHERITED, RID(), "Invalid text direction.");

	ShapedTextDataAdvanced* sd = memnew(ShapedTextDataAdvanced);
	sd->hb_buffer = hb_buffer_create();
	sd->direction = p_direction;
	sd->orientation = p_orientation;
	return shaped_owner.make_rid(sd);
}

void TextServerAdvanced::_shaped_text_set_direction(
	const RID& p_shaped, TextServer::Direction p_direction)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_COND_MSG(p_direction == DIRECTION_INHERITED, "Invalid text direction.");
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->direction != p_direction) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->direction = p_direction;
		invalidate(sd, false);
	}
}

TextServer::Direction TextServerAdvanced::_shaped_text_get_direction(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);

	MutexLock lock(sd->mutex);
	return sd->direction;
}

TextServer::Direction TextServerAdvanced::_shaped_text_get_inferred_direction(
	const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);

	MutexLock lock(sd->mutex);
	return sd->para_direction;
}

void TextServerAdvanced::_shaped_text_set_custom_punctuation(
	const RID& p_shaped, const String& p_punct)
{
	_THREAD_SAFE_METHOD_
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	if (sd->custom_punct != p_punct) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->custom_punct = p_punct;
		invalidate(sd, false);
	}
}

String TextServerAdvanced::_shaped_text_get_custom_punctuation(const RID& p_shaped) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	return sd->custom_punct;
}

void TextServerAdvanced::_shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char)
{
	_THREAD_SAFE_METHOD_
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);
	sd->el_char = p_char;
}

int64_t TextServerAdvanced::_shaped_text_get_custom_ellipsis(const RID& p_shaped) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	return sd->el_char;
}

void TextServerAdvanced::_shaped_text_set_orientation(
	const RID& p_shaped, TextServer::Orientation p_orientation)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->orientation != p_orientation) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->orientation = p_orientation;
		invalidate(sd, false);
	}
}

void TextServerAdvanced::_shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	ERR_FAIL_COND(sd->parent != RID());
	if (sd->preserve_invalid != p_enabled) {
		sd->preserve_invalid = p_enabled;
		invalidate(sd, false);
	}
}

bool TextServerAdvanced::_shaped_text_get_preserve_invalid(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	return sd->preserve_invalid;
}

void TextServerAdvanced::_shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->preserve_control != p_enabled) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->preserve_control = p_enabled;
		invalidate(sd, false);
	}
}

bool TextServerAdvanced::_shaped_text_get_preserve_control(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	return sd->preserve_control;
}

void TextServerAdvanced::_shaped_text_set_spacing(
	const RID& p_shaped, SpacingType p_spacing, int64_t p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, 4);
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->extra_spacing[p_spacing] != p_value) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->extra_spacing[p_spacing] = p_value;
		invalidate(sd, false);
	}
}

int64_t TextServerAdvanced::_shaped_text_get_spacing(
	const RID& p_shaped, SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, 4, 0);

	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	MutexLock lock(sd->mutex);
	return sd->extra_spacing[p_spacing];
}

TextServer::Orientation TextServerAdvanced::_shaped_text_get_orientation(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::ORIENTATION_HORIZONTAL);

	MutexLock lock(sd->mutex);
	return sd->orientation;
}

int64_t TextServerAdvanced::_shaped_get_span_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	if (sd->parent != RID()) {
		return sd->last_span - sd->first_span + 1;
	}
	else {
		return sd->spans.size();
	}
}

String TextServerAdvanced::_shaped_get_span_text(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	ShapedTextDataAdvanced* span_sd = sd;
	if (sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(sd->parent);
		ERR_FAIL_NULL_V(span_sd, String());
	}
	ERR_FAIL_INDEX_V(p_index, span_sd->spans.size(), String());
	return span_sd->text.substr(
		span_sd->spans[p_index].start, span_sd->spans[p_index].end - span_sd->spans[p_index].start);
}

void TextServerAdvanced::_generate_runs(ShapedTextDataAdvanced* p_sd) const
{
	ERR_FAIL_NULL(p_sd);
	p_sd->runs.clear();

	ShapedTextDataAdvanced* span_sd = p_sd;
	if (p_sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(p_sd->parent);
		ERR_FAIL_NULL(span_sd);
	}

	int sd_size = p_sd->glyphs.size();
	const Glyph* sd_gl = p_sd->glyphs.ptr();

	int span_count = span_sd->spans.size();
	int span = -1;
	int span_start = -1;
	int span_end = -1;

	TextRun run;
	for (int i = 0; i < sd_size; i += sd_gl[i].count) {
		const Glyph& gl = sd_gl[i];
		if (gl.start < 0 || gl.end < 0) {
			continue;
		}
		if (gl.start < span_start || gl.start >= span_end) {
			span = -1;
			span_start = -1;
			span_end = -1;
			for (int j = 0; j < span_count; j++) {
				if (gl.start >= span_sd->spans[j].start && gl.end <= span_sd->spans[j].end) {
					span = j;
					span_start = span_sd->spans[j].start;
					span_end = span_sd->spans[j].end;
					break;
				}
			}
		}
		if (run.font_rid != gl.font_rid || run.font_size != gl.font_size ||
			run.span_index != span || run.rtl != bool(gl.flags & GRAPHEME_IS_RTL)) {
			if (run.span_index >= 0) {
				p_sd->runs.push_back(run);
			}
			run.range = Vector2i(gl.start, gl.end);
			run.gl_range = Vector2i(i, i);
			run.font_rid = gl.font_rid;
			run.font_size = gl.font_size;
			run.rtl = bool(gl.flags & GRAPHEME_IS_RTL);
			run.span_index = span;
		}
		run.range.x = MIN(run.range.x, gl.start);
		run.range.y = MAX(run.range.y, gl.end);
		run.gl_range.x = MIN(run.gl_range.x, i);
		run.gl_range.y = MAX(run.gl_range.y, i);
	}
	if (run.span_index >= 0) {
		p_sd->runs.push_back(run);
	}
	p_sd->runs_dirty = false;
}

int64_t TextServerAdvanced::_shaped_get_run_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	return sd->runs.size();
}

String TextServerAdvanced::_shaped_get_run_text(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), String());
	return sd->text.substr(sd->runs[p_index].range.x - sd->start,
		sd->runs[p_index].range.y - sd->runs[p_index].range.x);
}

Vector2i TextServerAdvanced::_shaped_get_run_range(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), Vector2i());
	return sd->runs[p_index].range;
}

Vector2i TextServerAdvanced::_shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), Vector2i());
	return sd->runs[p_index].gl_range;
}

RID TextServerAdvanced::_shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), RID());
	return sd->runs[p_index].font_rid;
}

int TextServerAdvanced::_shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), 0);
	return sd->runs[p_index].font_size;
}

String TextServerAdvanced::_shaped_get_run_language(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), String());

	int span_idx = sd->runs[p_index].span_index;
	ShapedTextDataAdvanced* span_sd = sd;
	if (sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(sd->parent);
		ERR_FAIL_NULL_V(span_sd, String());
	}
	ERR_FAIL_INDEX_V(span_idx, span_sd->spans.size(), String());
	return span_sd->spans[span_idx].language;
}

TextServer::Direction TextServerAdvanced::_shaped_get_run_direction(
	const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), TextServer::DIRECTION_LTR);
	return sd->runs[p_index].rtl ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR;
}

String TextServerAdvanced::_shaped_get_text(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());

	return sd->text;
}

RID TextServerAdvanced::_shaped_text_substr(
	const RID& p_shaped, int64_t p_start, int64_t p_length) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());

	MutexLock lock(sd->mutex);
	if (sd->parent != RID()) {
		return _shaped_text_substr(sd->parent, p_start, p_length);
	}
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	ERR_FAIL_COND_V(p_start < 0 || p_length < 0, RID());
	ERR_FAIL_COND_V(sd->start > p_start || sd->end < p_start, RID());
	ERR_FAIL_COND_V(sd->end < p_start + p_length, RID());

	ShapedTextDataAdvanced* new_sd = memnew(ShapedTextDataAdvanced);
	new_sd->parent = p_shaped;
	new_sd->start = p_start;
	new_sd->end = p_start + p_length;
	new_sd->orientation = sd->orientation;
	new_sd->direction = sd->direction;
	new_sd->custom_punct = sd->custom_punct;
	new_sd->para_direction = sd->para_direction;
	new_sd->base_para_direction = sd->base_para_direction;
	for (int i = 0; i < TextServer::SPACING_MAX; i++) {
		new_sd->extra_spacing[i] = sd->extra_spacing[i];
	}

	if (!_shape_substr(new_sd, sd, p_start, p_length)) {
		memdelete(new_sd);
		return RID();
	}
	return shaped_owner.make_rid(new_sd);
}

RID TextServerAdvanced::_shaped_text_get_parent(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());

	MutexLock lock(sd->mutex);
	return sd->parent;
}

bool TextServerAdvanced::_shaped_text_shape(const RID& p_shaped) { return true; }

double TextServerAdvanced::_shaped_text_tab_align(
	const RID& p_shaped, const PackedFloat32Array& p_tab_stops)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}
	if (!sd->line_breaks_valid) {
		_shaped_text_update_breaks(p_shaped);
	}

	for (int i = 0; i < p_tab_stops.size(); i++) {
		if (p_tab_stops[i] <= 0) {
			return 0.0;
		}
	}

	int tab_index = 0;
	double off = 0.0;

	int start, end, delta;
	if (sd->para_direction == DIRECTION_LTR) {
		start = 0;
		end = sd->glyphs.size();
		delta = +1;
	}
	else {
		start = sd->glyphs.size() - 1;
		end = -1;
		delta = -1;
	}

	Glyph* gl = sd->glyphs.ptr();

	for (int i = start; i != end; i += delta) {
		if ((gl[i].flags & GRAPHEME_IS_TAB) == GRAPHEME_IS_TAB) {
			double tab_off = 0.0;
			while (tab_off <= off) {
				tab_off += p_tab_stops[tab_index];
				tab_index++;
				if (tab_index >= p_tab_stops.size()) {
					tab_index = 0;
				}
			}
			double old_adv = gl[i].advance;
			gl[i].advance = tab_off - off;
			sd->width += gl[i].advance - old_adv;
			off = 0;
			continue;
		}
		off += gl[i].advance * gl[i].repeat;
	}

	return 0.0;
}

int64_t TextServerAdvanced::_shaped_text_get_trim_pos(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, -1, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.trim_pos;
}

int64_t TextServerAdvanced::_shaped_text_get_ellipsis_pos(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, -1, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_pos;
}

const Glyph* TextServerAdvanced::_shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, nullptr, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_glyph_buf.ptr();
}

int64_t TextServerAdvanced::_shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, 0, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_glyph_buf.size();
}

void TextServerAdvanced::_update_chars(ShapedTextDataAdvanced* p_sd) const
{
	if (!p_sd->chars_valid) {
		p_sd->chars.clear();

		const UChar* data = p_sd->utf16.get_data();
		UErrorCode err = U_ZERO_ERROR;
		int prev = -1;
		int i = 0;

		Vector<ShapedTextDataAdvanced::Span>& spans = p_sd->spans;
		if (p_sd->parent != RID()) {
			ShapedTextDataAdvanced* parent_sd = shaped_owner.get_or_null(p_sd->parent);
			ERR_FAIL_COND(!parent_sd->valid.is_set());
			spans = parent_sd->spans;
		}

		int span_size = spans.size();
		while (i < span_size) {
			if (spans[i].start > p_sd->end) {
				break;
			}
			if (spans[i].end < p_sd->start) {
				i++;
				continue;
			}

			int r_start = MAX(0, spans[i].start - p_sd->start);
			String language = spans[i].language;
			while (i + 1 < span_size && language == spans[i + 1].language) {
				i++;
			}
			int r_end = MIN(spans[i].end - p_sd->start, p_sd->text.length());
			UBreakIterator* bi = ubrk_open(UBRK_CHARACTER,
				(language.is_empty())
					? TranslationServer::get_tool_locale().ascii().get_data()
					: language.ascii().get_data(),
				data + _convert_pos_inv(p_sd, r_start), _convert_pos_inv(p_sd, r_end - r_start),
				&err);
			if (U_SUCCESS(err)) {
				while (ubrk_next(bi) != UBRK_DONE) {
					int pos = _convert_pos(p_sd, ubrk_current(bi)) + r_start + p_sd->start;
					if (prev != pos) {
						p_sd->chars.push_back(pos);
					}
					prev = pos;
				}
				ubrk_close(bi);
			}
			else {
				for (int j = r_start; j < r_end; j++) {
					if (prev != j) {
						p_sd->chars.push_back(j + 1 + p_sd->start);
					}
					prev = j;
				}
			}
			i++;
		}
		p_sd->chars_valid = true;
	}
}

PackedInt32Array TextServerAdvanced::_shaped_text_get_character_breaks(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, PackedInt32Array());

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	_update_chars(sd);

	return sd->chars;
}

_FORCE_INLINE_ int64_t TextServerAdvanced::_generate_kashida_justification_opportunities(
	const String& p_data, int64_t p_start, int64_t p_end)
{
	int64_t kashida_pos = -1;
	int8_t priority = 100;
	int64_t i = p_start;

	char32_t pc = 0;

	while ((p_end > p_start) && is_transparent(p_data[p_end - 1])) {
		p_end--;
	}

	while (i < p_end) {
		uint32_t c = p_data[i];

		if (c == 0x0640) {
			kashida_pos = i;
			priority = 0;
		}
		if (priority >= 1 && i < p_end - 1) {
			if (is_seen_sad(c) && (p_data[i + 1] != 0x200c)) {
				kashida_pos = i;
				priority = 1;
			}
		}
		if (priority >= 2 && i > p_start) {
			if (is_teh_marbuta(c) || is_dal(c) || (is_heh(c) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 2;
				}
			}
		}
		if (priority >= 3 && i > p_start) {
			if (is_alef(c) ||
				((is_lam(c) || is_tah(c) || is_kaf(c) || is_gaf(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 3;
				}
			}
		}
		if (priority >= 4 && i > p_start && i < p_end - 1) {
			if (is_beh(c)) {
				if (is_reh(p_data[i + 1]) || is_yeh(p_data[i + 1])) {
					if (is_connected_to_prev(c, pc)) {
						kashida_pos = i - 1;
						priority = 4;
					}
				}
			}
		}
		if (priority >= 5 && i > p_start) {
			if (is_waw(c) || ((is_ain(c) || is_qaf(c) || is_feh(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 5;
				}
			}
		}
		if (priority >= 6 && i > p_start) {
			if (is_reh(c)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 6;
				}
			}
		}
		if (!is_transparent(c)) {
			pc = c;
		}
		i++;
	}

	return kashida_pos;
}

bool TextServerAdvanced::_shaped_text_update_justification_ops(const RID& p_shaped)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}
	if (!sd->line_breaks_valid) {
		_shaped_text_update_breaks(p_shaped);
	}

	if (sd->justification_ops_valid) {
		return true; // Nothing to do.
	}

	const UChar* data = sd->utf16.get_data();
	int data_size = sd->utf16.length();

	if (!sd->js_ops_valid) {
		sd->jstops.clear();

		// Use ICU word iterator and custom kashida detection.
		UErrorCode err = U_ZERO_ERROR;
		UBreakIterator* bi = ubrk_open(UBRK_WORD, "", data, data_size, &err);
		if (U_FAILURE(err)) {
			// No data - use fallback.
			int limit = 0;
			for (int i = 0; i < sd->text.length(); i++) {
				if (is_whitespace(sd->text[i])) {
					int ks = _generate_kashida_justification_opportunities(sd->text, limit, i) +
							 sd->start;
					if (ks != -1) {
						sd->jstops[ks] = true;
					}
					limit = i + 1;
				}
			}
			int ks =
				_generate_kashida_justification_opportunities(sd->text, limit, sd->text.length()) +
				sd->start;
			if (ks != -1) {
				sd->jstops[ks] = true;
			}
		}
		else {
			int limit = 0;
			while (ubrk_next(bi) != UBRK_DONE) {
				if (ubrk_getRuleStatus(bi) != UBRK_WORD_NONE) {
					int i = _convert_pos(sd, ubrk_current(bi));
					sd->jstops[i + sd->start] = false;
					int ks = _generate_kashida_justification_opportunities(sd->text, limit, i);
					if (ks != -1) {
						sd->jstops[ks + sd->start] = true;
					}
					limit = i;
				}
			}
			ubrk_close(bi);
		}

		sd->js_ops_valid = true;
	}

	sd->sort_valid = false;
	sd->glyphs_logical.clear();

	Glyph* sd_glyphs = sd->glyphs.ptr();
	int sd_size = sd->glyphs.size();
	if (!sd->jstops.is_empty()) {
		for (int i = 0; i < sd_size; i++) {
			if (sd_glyphs[i].count > 0) {
				char32_t c = sd->text[sd_glyphs[i].start - sd->start];
				if (c == 0x0640 && sd_glyphs[i].start == sd_glyphs[i].end - 1) {
					sd_glyphs[i].flags |= GRAPHEME_IS_ELONGATION;
				}
				if (sd->jstops.has(sd_glyphs[i].start)) {
					if (c == 0xfffc || c == 0x00ad) {
						continue;
					}
					if (sd->jstops[sd_glyphs[i].start]) {
						if (c != 0x0640) {
							if (sd_glyphs[i].font_rid != RID()) {
								Glyph gl = _shape_single_glyph(sd, 0x0640, HB_SCRIPT_ARABIC,
									HB_DIRECTION_RTL, sd->glyphs[i].font_rid,
									sd->glyphs[i].font_size);
								if ((sd_glyphs[i].flags & GRAPHEME_IS_VALID) == GRAPHEME_IS_VALID) {
#if HB_VERSION_ATLEAST(5, 1, 0)
									if ((i > 0) && ((sd_glyphs[i - 1].flags &
														GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL) !=
													   GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL)) {
										continue;
									}
#endif
									gl.start = sd_glyphs[i].start;
									gl.end = sd_glyphs[i].end;
									gl.repeat = 0;
									gl.count = 1;
									if (sd->orientation == ORIENTATION_HORIZONTAL) {
										gl.y_off = sd_glyphs[i].y_off;
									}
									else {
										gl.x_off = sd_glyphs[i].x_off;
									}
									gl.flags |= GRAPHEME_IS_ELONGATION | GRAPHEME_IS_VIRTUAL;
									sd->glyphs.insert(i, gl);
									i++;

									// Update write pointer and size.
									sd_size = sd->glyphs.size();
									sd_glyphs = sd->glyphs.ptr();
									continue;
								}
							}
						}
					}
					else if ((sd_glyphs[i].flags & GRAPHEME_IS_SPACE) != GRAPHEME_IS_SPACE &&
							   (sd_glyphs[i].flags & GRAPHEME_IS_PUNCTUATION) !=
								   GRAPHEME_IS_PUNCTUATION) {
						int count = sd_glyphs[i].count;
						// Do not add extra spaces at the end of the line.
						if (sd_glyphs[i].end == sd->end) {
							continue;
						}
						// Do not add extra space after existing space.
						if (sd_glyphs[i].flags & GRAPHEME_IS_RTL) {
							if ((i + count < sd_size - 1) &&
								((sd_glyphs[i + count].flags &
									 (GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT)) ==
									(GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT))) {
								continue;
							}
						}
						else {
							if ((i > 0) && ((sd_glyphs[i - 1].flags &
												(GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT)) ==
											   (GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT))) {
								continue;
							}
						}
						// Inject virtual space for alignment.
						Glyph gl;
						gl.span_index = sd_glyphs[i].span_index;
						gl.start = sd_glyphs[i].start;
						gl.end = sd_glyphs[i].end;
						gl.count = 1;
						gl.font_rid = sd_glyphs[i].font_rid;
						gl.font_size = sd_glyphs[i].font_size;
						gl.flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_VIRTUAL;
						if (sd_glyphs[i].flags & GRAPHEME_IS_RTL) {
							gl.flags |= GRAPHEME_IS_RTL;
							sd->glyphs.insert(i, gl); // Insert before.
						}
						else {
							sd->glyphs.insert(i + count, gl); // Insert after.
						}
						i += count;

						// Update write pointer and size.
						sd_size = sd->glyphs.size();
						sd_glyphs = sd->glyphs.ptr();
						continue;
					}
				}
			}
		}
	}

	sd->justification_ops_valid = true;
	return sd->justification_ops_valid;
}

Glyph TextServerAdvanced::_shape_single_glyph(ShapedTextDataAdvanced* p_sd, char32_t p_char,
	hb_script_t p_script, hb_direction_t p_direction, const RID& p_font, int64_t p_font_size)
{
	bool color = false;
	hb_font_t* hb_font = _font_get_hb_handle(p_font, p_font_size, color);
	double scale = _font_get_scale(p_font, p_font_size);
	bool subpos = (scale != 1.0) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_ONE_HALF) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_AUTO &&
					  p_font_size <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE);
	ERR_FAIL_NULL_V(hb_font, Glyph());

	hb_buffer_clear_contents(p_sd->hb_buffer);
	hb_buffer_set_direction(p_sd->hb_buffer, p_direction);
	hb_buffer_set_flags(p_sd->hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_DEFAULT));
	hb_buffer_set_script(
		p_sd->hb_buffer, (p_script == HB_TAG('Z', 's', 'y', 'e')) ? HB_SCRIPT_COMMON : p_script);
	hb_buffer_add_utf32(p_sd->hb_buffer, (const uint32_t*)&p_char, 1, 0, 1);

	hb_shape(hb_font, p_sd->hb_buffer, nullptr, 0);

	unsigned int glyph_count = 0;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(p_sd->hb_buffer, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(p_sd->hb_buffer, &glyph_count);

	// Process glyphs.
	Glyph gl;

	if (p_direction == HB_DIRECTION_RTL || p_direction == HB_DIRECTION_BTT) {
		gl.flags |= TextServer::GRAPHEME_IS_RTL;
	}

	gl.font_rid = p_font;
	gl.font_size = p_font_size;

	if (glyph_count > 0) {
		if (p_sd->orientation == ORIENTATION_HORIZONTAL) {
			if (subpos) {
				gl.advance = (double)glyph_pos[0].x_advance / (64.0 / scale) +
							 _get_extra_advance(p_font, p_font_size);
			}
			else {
				gl.advance = Math::round((double)glyph_pos[0].x_advance / (64.0 / scale) +
										 _get_extra_advance(p_font, p_font_size));
			}
		}
		else {
			gl.advance = -Math::round((double)glyph_pos[0].y_advance / (64.0 / scale));
		}
		gl.count = 1;

		gl.index = glyph_info[0].codepoint;
		if (subpos) {
			gl.x_off = (double)glyph_pos[0].x_offset / (64.0 / scale);
		}
		else {
			gl.x_off = Math::round((double)glyph_pos[0].x_offset / (64.0 / scale));
		}
		gl.y_off = -Math::round((double)glyph_pos[0].y_offset / (64.0 / scale));
		if (p_sd->orientation == ORIENTATION_HORIZONTAL) {
			gl.y_off += _font_get_baseline_offset(gl.font_rid) *
						(double)(_font_get_ascent(gl.font_rid, gl.font_size) +
								 _font_get_descent(gl.font_rid, gl.font_size));
		}
		else {
			gl.x_off += _font_get_baseline_offset(gl.font_rid) *
						(double)(_font_get_ascent(gl.font_rid, gl.font_size) +
								 _font_get_descent(gl.font_rid, gl.font_size));
		}

		if ((glyph_info[0].codepoint != 0) || !u_isgraph(p_char)) {
			gl.flags |= GRAPHEME_IS_VALID;
		}
	}
	return gl;
}

UBreakIterator* TextServerAdvanced::_create_line_break_iterator_for_locale(
	const String& p_language, UErrorCode* r_err) const
{
	// Creating UBreakIterator (ubrk_open) is surprisingly costly.
	// However, cloning (ubrk_clone) is cheaper, so we keep around blueprints to accelerate creating
	// new ones.

	String language =
		p_language.is_empty() ? TranslationServer::get_tool_locale() : p_language;
	if (!language.contains("@")) {
		if (lb_strictness == LB_LOOSE) {
			language += "@lb=loose";
		}
		else if (lb_strictness == LB_NORMAL) {
			language += "@lb=normal";
		}
		else if (lb_strictness == LB_STRICT) {
			language += "@lb=strict";
		}
	}

	_THREAD_SAFE_METHOD_
	const HashMap<String, UBreakIterator*>::Iterator key_value =
		line_break_iterators_per_language.find(language);
	if (key_value) {
		return ubrk_clone(key_value->value, r_err);
	}
	UBreakIterator* bi = ubrk_open(UBRK_LINE, language.ascii().get_data(), nullptr, 0, r_err);
	if (U_FAILURE(*r_err) || !bi) {
		return nullptr;
	}
	line_break_iterators_per_language.insert(language, bi);
	return ubrk_clone(bi, r_err);
}

bool TextServerAdvanced::_shaped_text_is_ready(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	// Atomic read is safe and faster.
	return sd->valid.is_set();
}

const Glyph* TextServerAdvanced::_shaped_text_get_glyphs(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, nullptr);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->glyphs.ptr();
}

int64_t TextServerAdvanced::_shaped_text_get_glyph_count(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->glyphs.size();
}

const Glyph* TextServerAdvanced::_shaped_text_sort_logical(const RID& p_shaped)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, nullptr);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}

	if (!sd->sort_valid) {
		sd->glyphs_logical = sd->glyphs;
		sd->glyphs_logical.sort_custom<GlyphCompare>();
		sd->sort_valid = true;
	}

	return sd->glyphs_logical.ptr();
}

Vector2i TextServerAdvanced::_shaped_text_get_range(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());

	MutexLock lock(sd->mutex);
	return Vector2(sd->start, sd->end);
}

Size2 TextServerAdvanced::_shaped_text_get_size(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Size2());

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->orientation == TextServer::ORIENTATION_HORIZONTAL) {
		return Size2((sd->text_trimmed ? sd->width_trimmed : sd->width),
			sd->ascent + sd->descent + sd->extra_spacing[SPACING_TOP] +
				sd->extra_spacing[SPACING_BOTTOM])
			.ceil();
	}
	else {
		return Size2(sd->ascent + sd->descent + sd->extra_spacing[SPACING_TOP] +
						 sd->extra_spacing[SPACING_BOTTOM],
			(sd->text_trimmed ? sd->width_trimmed : sd->width))
			.ceil();
	}
}

double TextServerAdvanced::_shaped_text_get_ascent(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->ascent + sd->extra_spacing[SPACING_TOP];
}

double TextServerAdvanced::_shaped_text_get_descent(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->descent + sd->extra_spacing[SPACING_BOTTOM];
}

double TextServerAdvanced::_shaped_text_get_width(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return Math::ceil(sd->text_trimmed ? sd->width_trimmed : sd->width);
}

double TextServerAdvanced::_shaped_text_get_underline_position(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	return sd->upos;
}

double TextServerAdvanced::_shaped_text_get_underline_thickness(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	return sd->uthk;
}

int64_t TextServerAdvanced::_is_confusable(
	const String& p_string, const PackedStringArray& p_dict) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return -1;
	}
#endif
	UErrorCode status = U_ZERO_ERROR;
	int64_t match_index = -1;

	Char16String utf16 = p_string.utf16();
	Vector<UChar*> skeletons;
	skeletons.resize(p_dict.size());

	if (sc_conf == nullptr) {
		sc_conf = uspoof_open(&status);
		uspoof_setChecks(sc_conf, USPOOF_CONFUSABLE, &status);
	}
	for (int i = 0; i < p_dict.size(); i++) {
		Char16String word = p_dict[i].utf16();
		int32_t len = uspoof_getSkeleton(sc_conf, 0, word.get_data(), -1, nullptr, 0, &status);
		skeletons.write[i] = (UChar*)memalloc(++len * sizeof(UChar));
		status = U_ZERO_ERROR;
		uspoof_getSkeleton(sc_conf, 0, word.get_data(), -1, skeletons.write[i], len, &status);
	}

	int32_t len = uspoof_getSkeleton(sc_conf, 0, utf16.get_data(), -1, nullptr, 0, &status);
	UChar* skel = (UChar*)memalloc(++len * sizeof(UChar));
	status = U_ZERO_ERROR;
	uspoof_getSkeleton(sc_conf, 0, utf16.get_data(), -1, skel, len, &status);
	for (int i = 0; i < skeletons.size(); i++) {
		if (u_strcmp(skel, skeletons[i]) == 0) {
			match_index = i;
			break;
		}
	}
	memfree(skel);

	for (int i = 0; i < skeletons.size(); i++) {
		memfree(skeletons.write[i]);
	}

	ERR_FAIL_COND_V_MSG(U_FAILURE(status), -1, u_errorName(status));

	return match_index;
}

bool TextServerAdvanced::_spoof_check(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return false;
	}
#endif
	UErrorCode status = U_ZERO_ERROR;
	Char16String utf16 = p_string.utf16();

	if (allowed == nullptr) {
		allowed = uset_openEmpty();
		uset_addAll(allowed, uspoof_getRecommendedSet(&status));
		uset_addAll(allowed, uspoof_getInclusionSet(&status));
	}
	if (sc_spoof == nullptr) {
		sc_spoof = uspoof_open(&status);
		uspoof_setAllowedChars(sc_spoof, allowed, &status);
		uspoof_setRestrictionLevel(sc_spoof, USPOOF_MODERATELY_RESTRICTIVE);
	}

	int32_t bitmask = uspoof_check(sc_spoof, utf16.get_data(), -1, nullptr, &status);
	ERR_FAIL_COND_V_MSG(U_FAILURE(status), false, u_errorName(status));

	return (bitmask != 0);
}

String TextServerAdvanced::_strip_diacritics(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return TextServer::strip_diacritics(p_string);
	}
#endif
	UErrorCode err = U_ZERO_ERROR;

	// Get NFKD normalizer singleton.
	const UNormalizer2* unorm = unorm2_getNFKDInstance(&err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), TextServer::strip_diacritics(p_string), u_errorName(err));

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	// Normalize.
	Vector<char16_t> normalized;
	err = U_ZERO_ERROR;
	int32_t len = unorm2_normalize(unorm, utf16.get_data(), -1, nullptr, 0, &err);
	ERR_FAIL_COND_V_MSG(
		err != U_BUFFER_OVERFLOW_ERROR, TextServer::strip_diacritics(p_string), u_errorName(err));
	normalized.resize(len);
	err = U_ZERO_ERROR;
	unorm2_normalize(unorm, utf16.get_data(), -1, normalized.ptrw(), len, &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), TextServer::strip_diacritics(p_string), u_errorName(err));

	// Convert back to UTF-32.
	String normalized_string = String::utf16(normalized.ptr(), len);

	// Strip combining characters.
	String result;
	for (int i = 0; i < normalized_string.length(); i++) {
		if (u_getCombiningClass(normalized_string[i]) == 0) {
			result = result + normalized_string[i];
		}
	}
	return result;
}

String TextServerAdvanced::_string_to_upper(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.to_upper();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> upper;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len = u_strToUpper(nullptr, 0, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	upper.resize(len);
	err = U_ZERO_ERROR;
	u_strToUpper(upper.ptrw(), len, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(upper.ptr(), len);
}

String TextServerAdvanced::_string_to_lower(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.to_lower();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> lower;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len = u_strToLower(nullptr, 0, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	lower.resize(len);
	err = U_ZERO_ERROR;
	u_strToLower(lower.ptrw(), len, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(lower.ptr(), len);
}

String TextServerAdvanced::_string_to_title(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.capitalize();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> upper;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len =
		u_strToTitle(nullptr, 0, utf16.get_data(), -1, nullptr, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	upper.resize(len);
	err = U_ZERO_ERROR;
	u_strToTitle(upper.ptrw(), len, utf16.get_data(), -1, nullptr, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(upper.ptr(), len);
}

PackedInt32Array TextServerAdvanced::_string_get_word_breaks(
	const String& p_string, const String& p_language, int64_t p_chars_per_line) const
{
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	HashSet<int> breaks;
	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator* bi = ubrk_open(
		UBRK_WORD, lang.ascii().get_data(), (const UChar*)utf16.get_data(), utf16.length(), &err);
	if (U_SUCCESS(err)) {
		while (ubrk_next(bi) != UBRK_DONE) {
			int pos = _convert_pos(p_string, utf16, ubrk_current(bi));
			if (pos != p_string.length() - 1) {
				breaks.insert(pos);
			}
		}
		ubrk_close(bi);
	}

	PackedInt32Array ret;

	if (p_chars_per_line > 0) {
		int line_start = 0;
		int last_break = -1;
		int line_length = 0;

		for (int i = 0; i < p_string.length(); i++) {
			const char32_t c = p_string[i];

			bool is_lb = is_linebreak(c);
			bool is_ws = is_whitespace(c);
			bool is_p =
				(u_ispunct(c) && c != 0x005F) || is_underscore(c) || c == '\t' || c == 0xfffc;

			if (is_lb) {
				if (line_length > 0) {
					ret.push_back(line_start);
					ret.push_back(i);
				}
				line_start = i;
				line_length = 0;
				last_break = -1;
				continue;
			}
			else if (breaks.has(i) || is_ws || is_p) {
				last_break = i;
			}

			if (line_length == p_chars_per_line) {
				if (last_break != -1) {
					int last_break_w_spaces = last_break;
					while (last_break > line_start && is_whitespace(p_string[last_break - 1])) {
						last_break--;
					}
					if (line_start != last_break) {
						ret.push_back(line_start);
						ret.push_back(last_break);
					}
					while (last_break_w_spaces < p_string.length() &&
						   is_whitespace(p_string[last_break_w_spaces])) {
						last_break_w_spaces++;
					}
					line_start = last_break_w_spaces;
					if (last_break_w_spaces < i) {
						line_length = i - last_break_w_spaces;
					}
					else {
						i = last_break_w_spaces;
						line_length = 0;
					}
				}
				else {
					ret.push_back(line_start);
					ret.push_back(i);
					line_start = i;
					line_length = 0;
				}
				last_break = -1;
			}
			line_length++;
		}
		if (line_length > 0) {
			ret.push_back(line_start);
			ret.push_back(p_string.length());
		}
	}
	else {
		int word_start = 0; // -1 if no word encountered. Leading spaces are part of a word.
		int word_length = 0;

		for (int i = 0; i < p_string.length(); i++) {
			const char32_t c = p_string[i];

			bool is_lb = is_linebreak(c);
			bool is_ws = is_whitespace(c);
			bool is_p =
				(u_ispunct(c) && c != 0x005F) || is_underscore(c) || c == '\t' || c == 0xfffc;

			if (word_start == -1) {
				if (!is_lb && !is_ws && !is_p) {
					word_start = i;
				}
				continue;
			}

			if (is_lb) {
				if (word_start != -1 && word_length > 0) {
					ret.push_back(word_start);
					ret.push_back(i);
				}
				word_start = -1;
				word_length = 0;
			}
			else if (breaks.has(i) || is_ws || is_p) {
				if (word_start != -1 && word_length > 0) {
					ret.push_back(word_start);
					ret.push_back(i);
				}
				if (is_ws || is_p) {
					word_start = -1;
				}
				else {
					word_start = i;
				}
				word_length = 0;
			}

			word_length++;
		}
		if (word_start != -1 && word_length > 0) {
			ret.push_back(word_start);
			ret.push_back(p_string.length());
		}
	}

	return ret;
}

PackedInt32Array TextServerAdvanced::_string_get_character_breaks(
	const String& p_string, const String& p_language) const
{
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	PackedInt32Array ret;

	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator* bi = ubrk_open(UBRK_CHARACTER, lang.ascii().get_data(),
		(const UChar*)utf16.get_data(), utf16.length(), &err);
	if (U_SUCCESS(err)) {
		while (ubrk_next(bi) != UBRK_DONE) {
			int pos = _convert_pos(p_string, utf16, ubrk_current(bi));
			ret.push_back(pos);
		}
		ubrk_close(bi);
	}
	else {
		return TextServer::string_get_character_breaks(p_string, p_language);
	}

	return ret;
}

bool TextServerAdvanced::_is_valid_identifier(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		WARN_PRINT_ONCE(
			"ICU data is not loaded, Unicode security and spoofing detection disabled.");
		return TextServer::is_valid_identifier(p_string);
	}
#endif

	enum UAX31SequenceStatus
	{
		SEQ_NOT_STARTED,
		SEQ_STARTED,
		SEQ_STARTED_VIR,
		SEQ_NEAR_END,
	};

	const char32_t* str = p_string.ptr();
	int len = p_string.length();

	if (len == 0) {
		return false; // Empty string.
	}

	UErrorCode err = U_ZERO_ERROR;
	Char16String utf16 = p_string.utf16();
	const UNormalizer2* norm_c = unorm2_getNFCInstance(&err);
	if (U_FAILURE(err)) {
		return false; // Failed to load normalizer.
	}
	bool isnurom = unorm2_isNormalized(norm_c, utf16.get_data(), utf16.length(), &err);
	if (U_FAILURE(err) || !isnurom) {
		return false; // Do not conform to Normalization Form C.
	}

	UAX31SequenceStatus A1_sequence_status = SEQ_NOT_STARTED;
	UScriptCode A1_scr = USCRIPT_INHERITED;
	UAX31SequenceStatus A2_sequence_status = SEQ_NOT_STARTED;
	UScriptCode A2_scr = USCRIPT_INHERITED;
	UAX31SequenceStatus B_sequence_status = SEQ_NOT_STARTED;
	UScriptCode B_scr = USCRIPT_INHERITED;

	for (int i = 0; i < len; i++) {
		err = U_ZERO_ERROR;
		UScriptCode scr = uscript_getScript(str[i], &err);
		if (U_FAILURE(err)) {
			return false; // Invalid script.
		}
		if (uscript_getUsage(scr) != USCRIPT_USAGE_RECOMMENDED) {
			return false; // Not a recommended script.
		}
		uint8_t cat = u_charType(str[i]);
		int32_t jt = u_getIntPropertyValue(str[i], UCHAR_JOINING_TYPE);

		// UAX #31 section 2.3 subsections A1, A2 and B, check ZWNJ and ZWJ usage.
		switch (A1_sequence_status) {
		case SEQ_NEAR_END: {
			if ((A1_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A1_scr)) {
				return false; // Mixed script.
			}
			if (jt == U_JT_RIGHT_JOINING || jt == U_JT_DUAL_JOINING) {
				A1_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else if (jt != U_JT_TRANSPARENT) {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED: {
			if ((A1_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A1_scr)) {
				A1_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (jt != U_JT_TRANSPARENT) {
					if (str[i] == 0x200C /*ZWNJ*/) {
						A1_sequence_status = SEQ_NEAR_END;
						continue;
					}
					else {
						A1_sequence_status = SEQ_NOT_STARTED; // Reset.
					}
				}
			}
		} break;
		default:
			break;
		}
		if (A1_sequence_status == SEQ_NOT_STARTED) {
			if (jt == U_JT_LEFT_JOINING || jt == U_JT_DUAL_JOINING) {
				A1_sequence_status = SEQ_STARTED;
				A1_scr = scr;
			}
		};

		switch (A2_sequence_status) {
		case SEQ_NEAR_END: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				return false; // Mixed script.
			}
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				A2_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED_VIR: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				A2_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (str[i] == 0x200C /*ZWNJ*/) {
					A2_sequence_status = SEQ_NEAR_END;
					continue;
				}
				else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
					A2_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		case SEQ_STARTED: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				A2_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (u_getCombiningClass(str[i]) == 9 /*Virama Combining Class*/) {
					A2_sequence_status = SEQ_STARTED_VIR;
				}
				else if (cat != U_MODIFIER_LETTER) {
					A2_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		default:
			break;
		}
		if (A2_sequence_status == SEQ_NOT_STARTED) {
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				A2_sequence_status = SEQ_STARTED;
				A2_scr = scr;
			}
		}

		switch (B_sequence_status) {
		case SEQ_NEAR_END: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				return false; // Mixed script.
			}
			if (u_getIntPropertyValue(str[i], UCHAR_INDIC_SYLLABIC_CATEGORY) !=
				U_INSC_VOWEL_DEPENDENT) {
				B_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED_VIR: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				B_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (str[i] == 0x200D /*ZWJ*/) {
					B_sequence_status = SEQ_NEAR_END;
					continue;
				}
				else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
					B_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		case SEQ_STARTED: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				B_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (u_getCombiningClass(str[i]) == 9 /*Virama Combining Class*/) {
					B_sequence_status = SEQ_STARTED_VIR;
				}
				else if (cat != U_MODIFIER_LETTER) {
					B_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		default:
			break;
		}
		if (B_sequence_status == SEQ_NOT_STARTED) {
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				B_sequence_status = SEQ_STARTED;
				B_scr = scr;
			}
		}

		if (u_hasBinaryProperty(str[i], UCHAR_PATTERN_SYNTAX) ||
			u_hasBinaryProperty(str[i], UCHAR_PATTERN_WHITE_SPACE) ||
			u_hasBinaryProperty(str[i], UCHAR_NONCHARACTER_CODE_POINT)) {
			return false; // Not a XID_Start or XID_Continue character.
		}
		if (i == 0) {
			if (!(cat == U_LOWERCASE_LETTER || cat == U_UPPERCASE_LETTER ||
					cat == U_TITLECASE_LETTER || cat == U_OTHER_LETTER ||
					cat == U_MODIFIER_LETTER || cat == U_LETTER_NUMBER || str[0] == 0x2118 ||
					str[0] == 0x212E || str[0] == 0x309B || str[0] == 0x309C || str[0] == 0x005F)) {
				return false; // Not a XID_Start character.
			}
		}
		else {
			if (!(cat == U_LOWERCASE_LETTER || cat == U_UPPERCASE_LETTER ||
					cat == U_TITLECASE_LETTER || cat == U_OTHER_LETTER ||
					cat == U_MODIFIER_LETTER || cat == U_LETTER_NUMBER ||
					cat == U_NON_SPACING_MARK || cat == U_COMBINING_SPACING_MARK ||
					cat == U_DECIMAL_DIGIT_NUMBER || cat == U_CONNECTOR_PUNCTUATION ||
					str[i] == 0x2118 || str[i] == 0x212E || str[i] == 0x309B || str[i] == 0x309C ||
					str[i] == 0x1369 || str[i] == 0x1371 || str[i] == 0x00B7 || str[i] == 0x0387 ||
					str[i] == 0x19DA || str[i] == 0x0E33 || str[i] == 0x0EB3 || str[i] == 0xFF9E ||
					str[i] == 0xFF9F)) {
				return false; // Not a XID_Continue character.
			}
		}
	}
	return true;
}

bool TextServerAdvanced::_is_valid_letter(uint64_t p_unicode) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return TextServer::is_valid_letter(p_unicode);
	}
#endif

	return u_isalpha(p_unicode);
}

void TextServerAdvanced::_font_clear_system_fallback_cache()
{
	_THREAD_SAFE_METHOD_
	for (const KeyValue<SystemFontKey, SystemFontCache>& E : system_fonts) {
		const Vector<SystemFontCacheRec>& sysf_cache = E.value.var;
		for (const SystemFontCacheRec& F : sysf_cache) {
			_free_rid(F.rid);
		}
	}
	system_fonts.clear();
	system_font_data.clear();
}

void TextServerAdvanced::_cleanup() { font_clear_system_fallback_cache(); }

TextServerAdvanced::~TextServerAdvanced()
{
	_bmp_free_font_funcs();
#ifdef MODULE_FREETYPE_ENABLED
	if (ft_library != nullptr) {
		FT_Done_FreeType(ft_library);
	}
#endif
	if (sc_spoof != nullptr) {
		uspoof_close(sc_spoof);
		sc_spoof = nullptr;
	}
	if (sc_conf != nullptr) {
		uspoof_close(sc_conf);
		sc_conf = nullptr;
	}
	if (allowed != nullptr) {
		uset_close(allowed);
		allowed = nullptr;
	}
	for (const KeyValue<String, UBreakIterator*>& bi : line_break_iterators_per_language) {
		ubrk_close(bi.value);
	}

	std::atexit(u_cleanup);
}

int TextServerAdvanced::ft_move_to(const FT_Vector* to, void* user) { return 0; }

int TextServerAdvanced::ft_line_to(const FT_Vector* to, void* user) { return 0; }

int TextServerAdvanced::ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
	return 0;
}

int TextServerAdvanced::ft_cubic_to(
	const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
	return 0;
}

bool TextServerAdvanced::_ensure_cache_for_size(FontAdvanced* p_font_data, const Vector2i& p_size,
	FontForSizeAdvanced*& r_cache_for_size, bool p_silent, uint32_t p_oversampling) const
{
	ERR_FAIL_NULL_V(p_font_data, false);
	ERR_FAIL_COND_V(p_size.x <= 0, false);

	HashMap<Vector2i, FontForSizeAdvanced*>::Iterator E = p_font_data->cache.find(p_size);
	if (E) {
#ifdef MODULE_FREETYPE_ENABLED
		if (E->value->fsize != nullptr) {
			FT_Activate_Size(E->value->fsize);
		}
#endif
		r_cache_for_size = E->value;
		if (p_oversampling == 0 && E->value->viewport_oversampling != 0) {
			OversamplingLevel* ol = oversampling_levels.getptr(E->value->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(E->value);
			}
		}
		return true;
	}

	FontForSizeAdvanced* fd = memnew(FontForSizeAdvanced);
	fd->size = p_size;
	if (p_font_data->data_ptr && (p_font_data->data_size > 0)) {
#ifdef MODULE_FREETYPE_ENABLED
		int error = 0;
		{
			MutexLock ftlock(ft_mutex);
			if (!ft_library) {
				error = FT_Init_FreeType(&ft_library);
				if (error != 0) {
					memdelete(fd);
					if (p_silent) {
						return false;
					}
					ERR_FAIL_V_MSG(false,
						"FreeType: Error initializing library: '" + String(FT_Error_String(error)) +
							"'.");
				}
#ifdef MODULE_SVG_ENABLED
				FT_Property_Set(ft_library, "ot-svg", "svg-hooks", get_tvg_svg_in_ot_hooks());
#endif
			}

			if (p_font_data->face == nullptr) {
				memset(&p_font_data->stream, 0, sizeof(FT_StreamRec));
				p_font_data->stream.base = (unsigned char*)p_font_data->data_ptr;
				p_font_data->stream.size = p_font_data->data_size;
				p_font_data->stream.pos = 0;

				FT_Open_Args fargs;
				memset(&fargs, 0, sizeof(FT_Open_Args));
				fargs.memory_base = (unsigned char*)p_font_data->data_ptr;
				fargs.memory_size = p_font_data->data_size;
				fargs.flags = FT_OPEN_MEMORY;
				fargs.stream = &p_font_data->stream;

				error = FT_Open_Face(
					ft_library, &fargs, p_font_data->face_index, &p_font_data->face);
				if (error) {
					if (p_font_data->face) {
						FT_Done_Face(p_font_data->face);
						p_font_data->face = nullptr;
					}
					memdelete(fd);
					if (p_silent) {
						return false;
					}
					ERR_FAIL_V_MSG(false,
						"FreeType: Error loading font: '" + String(FT_Error_String(error)) +
							"' (face_index=" + String::num_int64(p_font_data->face_index) + ").");
				}
			}

			FT_New_Size(p_font_data->face, &fd->fsize);
			FT_Activate_Size(fd->fsize);
		}

		double sz = double(fd->size.x) / 64.0;
		if (p_font_data->msdf) {
			sz = p_font_data->msdf_source_size;
		}

		if (FT_HAS_COLOR(p_font_data->face) && p_font_data->face->num_fixed_sizes > 0) {
			int best_match = 0;
			int diff = Math::abs(sz - ((int64_t)p_font_data->face->available_sizes[0].width));
			fd->scale = sz / p_font_data->face->available_sizes[0].width;
			for (int i = 1; i < p_font_data->face->num_fixed_sizes; i++) {
				int ndiff = Math::abs(sz - ((int64_t)p_font_data->face->available_sizes[i].width));
				if (ndiff < diff) {
					best_match = i;
					diff = ndiff;
					fd->scale = sz / p_font_data->face->available_sizes[i].width;
				}
			}
			FT_Select_Size(p_font_data->face, best_match);
		}
		else {
			FT_Size_RequestRec req;
			req.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
			req.width = MIN(2048.0, sz) * 64.0;
			req.height = MIN(2048.0, sz) * 64.0;
			req.horiResolution = 0;
			req.vertResolution = 0;

			FT_Request_Size(p_font_data->face, &req);
			if (p_font_data->face->size->metrics.y_ppem != 0) {
				fd->scale = sz / (double)p_font_data->face->size->metrics.y_ppem;
			}
		}

		fd->hb_handle = hb_ft_font_create(p_font_data->face, nullptr);

		fd->ascent = (p_font_data->face->size->metrics.ascender / 64.0) * fd->scale;
		fd->descent = (-p_font_data->face->size->metrics.descender / 64.0) * fd->scale;
		fd->underline_position =
			(-FT_MulFix(p_font_data->face->underline_position,
				 p_font_data->face->size->metrics.y_scale) /
				64.0) *
			fd->scale;
		fd->underline_thickness =
			(FT_MulFix(p_font_data->face->underline_thickness,
				 p_font_data->face->size->metrics.y_scale) /
				64.0) *
			fd->scale;

#if HB_VERSION_ATLEAST(3, 3, 0)
		hb_font_set_synthetic_slant(fd->hb_handle, p_font_data->transform[0][1]);
#endif

		if (!p_font_data->face_init) {
			if (p_font_data->font_name.is_empty() && p_font_data->face->family_name != nullptr) {
				p_font_data->font_name = String::utf8((const char*)p_font_data->face->family_name);
			}
			if (p_font_data->face->style_name != nullptr) {
				p_font_data->style_name = String::utf8((const char*)p_font_data->face->style_name);
			}
			p_font_data->weight = _font_get_weight_by_name(p_font_data->style_name.to_lower());
			p_font_data->stretch = _font_get_stretch_by_name(p_font_data->style_name.to_lower());
			p_font_data->style_flags = 0;
			if ((p_font_data->face->style_flags & FT_STYLE_FLAG_BOLD) ||
				p_font_data->weight >= 700) {
				p_font_data->style_flags |= FONT_BOLD;
			}
			if ((p_font_data->face->style_flags & FT_STYLE_FLAG_ITALIC) ||
				_is_ital_style(p_font_data->style_name.to_lower())) {
				p_font_data->style_flags |= FONT_ITALIC;
			}
			if (p_font_data->face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) {
				p_font_data->style_flags |= FONT_FIXED_WIDTH;
			}
			p_font_data->face_init = true;
		}
#else
		memdelete(fd);
		if (p_silent) {
			return false;
		}
		ERR_FAIL_V_MSG(false,
			"FreeType: Can't load dynamic font, engine is compiled without FreeType support!");
#endif
	}
	else {
		fd->hb_handle = _bmp_font_create(fd, nullptr);
	}

	fd->owner = p_font_data;
	p_font_data->cache.insert(p_size, fd);
	r_cache_for_size = fd;
	if (p_oversampling != 0) {
		OversamplingLevel* ol = oversampling_levels.getptr(p_oversampling);
		if (ol) {
			fd->viewport_oversampling = p_oversampling;
			ol->fonts.insert(fd);
		}
	}
	return true;
}

void TextServerAdvanced::full_copy(ShapedTextDataAdvanced* p_shaped) {}

bool TextServerAdvanced::_shaped_text_update_breaks(const RID& p_shaped) { return true; }

bool TextServerAdvanced::_shape_substr(ShapedTextDataAdvanced* p_new_sd,
	const ShapedTextDataAdvanced* p_sd, int64_t p_start, int64_t p_length) const
{
	return true;
}

// This implementation is derived from ICU: icu4c/source/extra/scrptrun/scrptrun.cpp

inline constexpr UChar32 ZERO_WIDTH_JOINER = 0x200d;
inline constexpr UChar32 VARIATION_SELECTOR_15 = 0xfe0e;
inline constexpr UChar32 VARIATION_SELECTOR_16 = 0xfe0f;
inline constexpr UChar32 COMBINING_ENCLOSING_KEYCAP = 0x20e3;

inline bool ScriptIterator::same_script(int32_t p_script_one, int32_t p_script_two) {
	return p_script_one <= USCRIPT_INHERITED || p_script_two <= USCRIPT_INHERITED || p_script_one == p_script_two;
}

inline bool ScriptIterator::is_emoji(UChar32 p_c, UChar32 p_next) {
	if (p_next == VARIATION_SELECTOR_15 && (u_hasBinaryProperty(p_c, UCHAR_EMOJI) || u_hasBinaryProperty(p_c, UCHAR_EXTENDED_PICTOGRAPHIC))) {
		return false;
	} else if (p_next == VARIATION_SELECTOR_16 && (u_hasBinaryProperty(p_c, UCHAR_EMOJI) || u_hasBinaryProperty(p_c, UCHAR_EXTENDED_PICTOGRAPHIC))) {
		return true;
	} else {
		return u_hasBinaryProperty(p_c, UCHAR_EMOJI_PRESENTATION) || u_hasBinaryProperty(p_c, UCHAR_EMOJI_MODIFIER) || u_hasBinaryProperty(p_c, UCHAR_REGIONAL_INDICATOR) || (u_hasBinaryProperty(p_c, UCHAR_EMOJI) && u_hasBinaryProperty(p_next, UCHAR_EMOJI_MODIFIER));
	}
}

ScriptIterator::ScriptIterator(const String &p_string, int p_start, int p_length) {
	struct ParenStackEntry {
		int pair_index;
		UScriptCode script_code;
	};

	struct EmojiSubrunEntry {
		int start;
		int end;
	};

	if (p_start >= p_length) {
		p_start = p_length - 1;
	}

	if (p_start < 0) {
		p_start = 0;
	}

	int paren_size = PAREN_STACK_DEPTH;
	ParenStackEntry starter_paren_stack[PAREN_STACK_DEPTH];
	ParenStackEntry *paren_stack = starter_paren_stack;

	int emoji_size = EMOJI_STACK_DEPTH;
	EmojiSubrunEntry starter_emoji_stack[EMOJI_STACK_DEPTH];
	EmojiSubrunEntry *emoji_stack = starter_emoji_stack;

	int script_start;
	int script_end = p_start;
	UScriptCode script_code;
	int paren_sp = -1;
	int start_sp = paren_sp;
	UErrorCode err = U_ZERO_ERROR;
	const char32_t *str = p_string.ptr();

	do {
		script_code = USCRIPT_COMMON;
		int emoji_sp = -1;
		bool emoji_run = false;
		for (script_start = script_end; script_end < p_length; script_end++) {
			UChar32 ch = str[script_end];
			UChar32 n = (script_end + 1 < p_length) ? str[script_end + 1] : 0;
			if (is_emoji(ch, n)) {
				if (!emoji_run) {
					emoji_run = true;
					emoji_sp++;
					if (unlikely(emoji_sp >= emoji_size)) {
						emoji_size += EMOJI_STACK_DEPTH;
						if (emoji_stack == starter_emoji_stack) {
							emoji_stack = static_cast<EmojiSubrunEntry *>(memalloc(emoji_size * sizeof(EmojiSubrunEntry)));
						} else {
							emoji_stack = static_cast<EmojiSubrunEntry *>(memrealloc(emoji_stack, emoji_size * sizeof(EmojiSubrunEntry)));
						}
					}
					emoji_stack[emoji_sp].start = script_end;
					emoji_stack[emoji_sp].end = script_end;
				}
			} else if (emoji_run && ch != ZERO_WIDTH_JOINER && ch != VARIATION_SELECTOR_16 && ch != COMBINING_ENCLOSING_KEYCAP && !(u_hasBinaryProperty(ch, UCHAR_EXTENDED_PICTOGRAPHIC) && n != VARIATION_SELECTOR_15)) {
				emoji_run = false;
				emoji_stack[emoji_sp].end = script_end;
			}

			UScriptCode sc = uscript_getScript(ch, &err);
			if (U_FAILURE(err)) {
				if (paren_stack != starter_paren_stack) {
					memfree(paren_stack);
				}
				ERR_FAIL_MSG(u_errorName(err));
			}

			if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) != U_BPT_NONE) {
				if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_OPEN) {
					// If it's an open character, push it onto the stack.
					paren_sp++;
					if (unlikely(paren_sp >= paren_size)) {
						// If the stack is full, allocate more space to handle deeply nested parentheses. This is unlikely to happen with any real text.
						paren_size += PAREN_STACK_DEPTH;
						if (paren_stack == starter_paren_stack) {
							paren_stack = static_cast<ParenStackEntry *>(memalloc(paren_size * sizeof(ParenStackEntry)));
						} else {
							paren_stack = static_cast<ParenStackEntry *>(memrealloc(paren_stack, paren_size * sizeof(ParenStackEntry)));
						}
					}
					paren_stack[paren_sp].pair_index = ch;
					paren_stack[paren_sp].script_code = script_code;
				} else if (paren_sp >= 0) {
					// If it's a close character, find the matching open on the stack, and use that script code. Any non-matching open characters above it on the stack will be popped.
					UChar32 paired_ch = u_getBidiPairedBracket(ch);
					while (paren_sp >= 0 && paren_stack[paren_sp].pair_index != paired_ch) {
						paren_sp -= 1;
					}
					if (paren_sp < start_sp) {
						start_sp = paren_sp;
					}
					if (paren_sp >= 0) {
						sc = paren_stack[paren_sp].script_code;
					}
				}
			}

			if (same_script(script_code, sc)) {
				if (script_code <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
					script_code = sc;
					// Now that we have a final script code, fix any open characters we pushed before we knew the script code.
					while (start_sp < paren_sp) {
						paren_stack[++start_sp].script_code = script_code;
					}
				}
				if ((u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_CLOSE) && paren_sp >= 0) {
					// If this character is a close paired character pop the matching open character from the stack.
					paren_sp -= 1;
					if (start_sp >= 0) {
						start_sp -= 1;
					}
				}
			} else {
				break;
			}
		}
		if (emoji_run) {
			emoji_stack[emoji_sp].end = script_end;
		}

		for (int sub = 0; sub <= emoji_sp; sub++) {
			if (emoji_stack[sub].start > script_start) {
				ScriptRange rng;
				rng.script = hb_icu_script_to_script(script_code);
				rng.start = script_start;
				rng.end = emoji_stack[sub].start;
				script_ranges.push_back(rng);
			}
			ScriptRange rng;
			rng.script = (hb_script_t)HB_TAG('Z', 's', 'y', 'e');
			rng.start = emoji_stack[sub].start;
			rng.end = emoji_stack[sub].end;
			script_ranges.push_back(rng);

			script_start = emoji_stack[sub].end;
		}
		if (script_start != script_end) {
			ScriptRange rng;
			rng.script = hb_icu_script_to_script(script_code);
			rng.start = script_start;
			rng.end = script_end;
			script_ranges.push_back(rng);
		}

		if (emoji_stack != starter_emoji_stack) {
			memfree(emoji_stack);
		}
	} while (script_end < p_length);

	if (paren_stack != starter_paren_stack) {
		memfree(paren_stack);
	}
}

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED

#include <freetype/otsvg.h>
#include <ft2build.h>

FT_Error tvg_svg_in_ot_init(FT_Pointer *p_state) {
	*p_state = memnew(TVG_State);

	return FT_Err_Ok;
}

void tvg_svg_in_ot_free(FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	memdelete(state);
}

static void construct_xml(Ref<XMLParser> &parser, double &r_embox_x, double &r_embox_y, String *p_xml, int64_t &r_tag_count) {
	if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
		*p_xml += vformat("<%s", parser->get_node_name());
		bool is_svg_tag = parser->get_node_name() == "svg";
		for (int i = 0; i < parser->get_attribute_count(); i++) {
			String aname = parser->get_attribute_name(i);
			String value = parser->get_attribute_value(i);
			if (is_svg_tag && aname == "viewBox") {
				PackedStringArray vb = value.split(" ");
				if (vb.size() == 4) {
					r_embox_x = vb[2].to_float();
					r_embox_y = vb[3].to_float();
				}
			} else if (is_svg_tag && aname == "width") {
				r_embox_x = value.to_float();
			} else if (is_svg_tag && aname == "height") {
				r_embox_y = value.to_float();
			} else {
				*p_xml += vformat(" %s=\"%s\"", aname, value);
			}
		}

		if (parser->is_empty()) {
			*p_xml += "/>";
		} else {
			*p_xml += ">";
			if (r_tag_count >= 0) {
				r_tag_count++;
			}
		}
	} else if (parser->get_node_type() == XMLParser::NODE_TEXT) {
		*p_xml += parser->get_node_data();
	} else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
		*p_xml += vformat("</%s>", parser->get_node_name());
		if (r_tag_count > 0) {
			r_tag_count--;
		}
	}
}

FT_Error tvg_svg_in_ot_preset_slot(FT_GlyphSlot p_slot, FT_Bool p_cache, FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	if (!state) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG in OT state not initialized.");
	}
	MutexLock lock(state->mutex);

	FT_SVG_Document document = (FT_SVG_Document)p_slot->other;
	FT_Size_Metrics metrics = document->metrics;

	GL_State &gl_state = state->glyph_map[p_slot->glyph_index];
	if (!gl_state.ready) {
		Ref<XMLParser> parser;
		parser.instantiate();
		parser->_open_buffer((const uint8_t *)document->svg_document, document->svg_document_length);

		String xml_body;

		double embox_x = document->units_per_EM;
		double embox_y = document->units_per_EM;

		TVG_DocumentCache &cache = state->document_map[document->svg_document];

		if (!cache.xml_body.is_empty()) {
			// If we have a cached document, that means we have already parsed it.
			// All node cache should be available.

			xml_body = cache.xml_body;
			embox_x = cache.embox_x;
			embox_y = cache.embox_y;

			ERR_FAIL_COND_V(!cache.node_caches.has(p_slot->glyph_index), FT_Err_Invalid_SVG_Document);
			Vector<TVG_NodeCache> &ncs = cache.node_caches[p_slot->glyph_index];

			uint64_t offset = 0;
			for (TVG_NodeCache &nc : ncs) {
				// Seek will call read() internally.
				if (parser->seek(nc.document_offset) == OK) {
					int64_t tag_count = 0;
					String xml_node;

					// We only parse the glyph node.
					do {
						construct_xml(parser, embox_x, embox_y, &xml_node, tag_count);
					} while (tag_count != 0 && parser->read() == OK);

					xml_body = xml_body.insert(nc.body_offset + offset, xml_node);
					offset += xml_node.length();
				}
			}
		} else {
			String xml_node;
			String xml_body_temp;

			String *p_xml = &xml_body_temp;
			int64_t tag_count = -1;

			bool is_in_defs = false;
			while (parser->read() == OK) {
				if (parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name().to_lower() == "defs") {
					is_in_defs = true;
				} else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END && parser->get_node_name().to_lower() == "defs") {
					is_in_defs = false;
				}
				if (!is_in_defs && parser->has_attribute("id")) {
					const String &gl_name = parser->get_named_attribute_value("id");
					if (gl_name.begins_with("glyph")) {
						int dot_pos = gl_name.find_char('.');
						int64_t gl_idx = gl_name.substr(5, (dot_pos > 0) ? dot_pos - 5 : -1).to_int();

						TVG_NodeCache node_cache = TVG_NodeCache();
						node_cache.document_offset = parser->get_node_offset(),
						node_cache.body_offset = (uint64_t)cache.xml_body.length();
						cache.node_caches[gl_idx].push_back(node_cache);

						if (p_slot->glyph_index != gl_idx) {
							parser->skip_section();
							continue;
						}
						tag_count = 0;
						xml_node = "";
						p_xml = &xml_node;
					}
				}

				xml_body_temp = "";
				construct_xml(parser, embox_x, embox_y, p_xml, tag_count);

				if (xml_body_temp.length() > 0) {
					xml_body += xml_body_temp;
					cache.xml_body += xml_body_temp;
					continue;
				}

				if (tag_count == 0) {
					p_xml = &xml_body_temp;
					tag_count = -1;
					xml_body += xml_node;
				}
			}

			cache.embox_x = embox_x;
			cache.embox_y = embox_y;
		}

		tvg::Picture *picture = tvg::Picture::gen();
		gl_state.xml_code = xml_body.utf8();

		tvg::Result result = picture->load(gl_state.xml_code.get_data(), gl_state.xml_code.length(), "svg+xml", nullptr, false);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to load SVG document (glyph metrics).");
		}

		float svg_width, svg_height;
		picture->size(&svg_width, &svg_height);
		double aspect_x = (svg_width > svg_height) ? svg_width / svg_height : 1.0;
		double aspect_y = (svg_width < svg_height) ? svg_height / svg_width : 1.0;

		result = picture->size(embox_x * aspect_x, embox_y * aspect_y);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
		}

		double scale = double(embox_x * aspect_x) / double(svg_width);
		double yoff = double(document->metrics.ascender + document->metrics.descender) / double(document->metrics.ascender);

		double x_svg_to_out = (double)metrics.x_ppem / embox_x / scale;
		double y_svg_to_out = (double)metrics.y_ppem / embox_y / scale;

		gl_state.m.e11 = (double)document->transform.xx / (1 << 16);
		gl_state.m.e12 = -(double)document->transform.xy / (1 << 16);
		gl_state.m.e21 = -(double)document->transform.yx / (1 << 16);
		gl_state.m.e22 = (double)document->transform.yy / (1 << 16);
		gl_state.m.e13 = (double)document->delta.x / 64 * embox_x / metrics.x_ppem * scale;
		gl_state.m.e23 = -(double)document->delta.y / 64 * embox_y / metrics.y_ppem * scale;
		gl_state.m.e31 = 0;
		gl_state.m.e32 = 0;
		gl_state.m.e33 = 1;

		result = picture->size(embox_x * aspect_x * x_svg_to_out, embox_y * aspect_y * y_svg_to_out);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
		}

		result = picture->transform(gl_state.m);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to apply transform to SVG document.");
		}

		picture->size(&gl_state.w, &gl_state.h);

		gl_state.x = (double(p_slot->metrics.horiAdvance) / 64.0 - gl_state.w) / 2.0;
		gl_state.y = -Math::ceil(gl_state.h * yoff);

		tvg::Paint::rel(picture);

		gl_state.ready = true;
	}

	p_slot->bitmap_left = (FT_Int)gl_state.x;
	p_slot->bitmap_top = (FT_Int)-gl_state.y;

	double tmpd = Math::ceil(gl_state.h);
	p_slot->bitmap.rows = (unsigned int)tmpd;
	tmpd = Math::ceil(gl_state.w);
	p_slot->bitmap.width = (unsigned int)tmpd;
	p_slot->bitmap.pitch = (int)p_slot->bitmap.width * 4;

	p_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

	float metrics_width = (float)gl_state.w;
	float metrics_height = (float)gl_state.h;

	float horiBearingX = (float)gl_state.x;
	float horiBearingY = (float)-gl_state.y;

	float vertBearingX = p_slot->metrics.horiBearingX / 64.0f - p_slot->metrics.horiAdvance / 64.0f / 2;
	float vertBearingY = (p_slot->metrics.vertAdvance / 64.0f - p_slot->metrics.height / 64.0f) / 2;

	float tmpf = Math::round(metrics_width * 64);
	p_slot->metrics.width = (FT_Pos)tmpf;
	tmpf = Math::round(metrics_height * 64);
	p_slot->metrics.height = (FT_Pos)tmpf;

	p_slot->metrics.horiBearingX = (FT_Pos)(horiBearingX * 64);
	p_slot->metrics.horiBearingY = (FT_Pos)(horiBearingY * 64);
	p_slot->metrics.vertBearingX = (FT_Pos)(vertBearingX * 64);
	p_slot->metrics.vertBearingY = (FT_Pos)(vertBearingY * 64);

	if (p_slot->metrics.vertAdvance == 0) {
		p_slot->metrics.vertAdvance = (FT_Pos)(metrics_height * 1.2f * 64);
	}

	return FT_Err_Ok;
}

FT_Error tvg_svg_in_ot_render(FT_GlyphSlot p_slot, FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	if (!state) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG in OT state not initialized.");
	}
	MutexLock lock(state->mutex);

	if (!state->glyph_map.has(p_slot->glyph_index)) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG glyph not loaded.");
	}

	GL_State &gl_state = state->glyph_map[p_slot->glyph_index];
	ERR_FAIL_COND_V_MSG(!gl_state.ready, FT_Err_Invalid_SVG_Document, "SVG glyph not ready.");

	tvg::Picture *picture = tvg::Picture::gen();
	tvg::Result res = picture->load(gl_state.xml_code.get_data(), gl_state.xml_code.length(), "svg+xml", nullptr, false);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to load SVG document (glyph rendering).");
	}
	res = picture->size(gl_state.w, gl_state.h);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
	}
	res = picture->transform(gl_state.m);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to apply transform to SVG document.");
	}

	std::unique_ptr<tvg::SwCanvas> sw_canvas(tvg::SwCanvas::gen());
	res = sw_canvas->target((uint32_t *)p_slot->bitmap.buffer, (int)p_slot->bitmap.width, (int)p_slot->bitmap.width, (int)p_slot->bitmap.rows, tvg::ColorSpace::ARGB8888S);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to create SVG canvas.");
	}
	res = sw_canvas->add(picture);
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to set SVG canvas source.");
	}
	res = sw_canvas->draw();
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to draw to SVG canvas.");
	}
	res = sw_canvas->sync();
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to sync SVG canvas.");
	}

	state->glyph_map.erase(p_slot->glyph_index);

	p_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
	p_slot->bitmap.num_grays = 256;
	p_slot->format = FT_GLYPH_FORMAT_BITMAP;

	return FT_Err_Ok;
}

SVG_RendererHooks tvg_svg_in_ot_hooks = {
	(SVG_Lib_Init_Func)tvg_svg_in_ot_init,
	(SVG_Lib_Free_Func)tvg_svg_in_ot_free,
	(SVG_Lib_Render_Func)tvg_svg_in_ot_render,
	(SVG_Lib_Preset_Slot_Func)tvg_svg_in_ot_preset_slot,
};

SVG_RendererHooks *get_tvg_svg_in_ot_hooks() {
	return &tvg_svg_in_ot_hooks;
}

#endif // MODULE_FREETYPE_ENABLED
#endif // MODULE_SVG_ENABLED

void initialize_text_server_adv_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
	if (TextServerManager::is_initialized()) {
		Ref<TextServerAdvanced> ts;
		ts.instantiate();
		TextServerManager::add_interface(ts);
	}
}

void uninitialize_text_server_adv_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
}
