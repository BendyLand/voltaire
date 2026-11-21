/**************************************************************************/
/*  range.cpp                                                             */
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

#include <cmath>
#include <thirdparty/misc/r128.h>
#include "core/string/translation_server.h"
#include "range.h"
#include "core/config/engine.h"
#include "progress_bar.h"
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"
#include "texture_progress_bar.h"
#include "core/input/input.h"
#include "slider.h"
#include "core/math/expression.h"
#include "spin_box.h"
#include "scene/main/window.h"
#include "scroll_bar.h"
#include "servers/display/display_server.h"

double NumericRange::snapped_r128(double p_value, double p_step)
{
	if (p_step == 0.0) {
		return p_value;
	}
	if (p_value > (1e18 * p_step) || p_value < -(1e18 * p_step)) {
		return Math::snapped(p_value, p_step);
	}
	const int decimals = MIN(18, 14 - Math::floor(std::log10(MAX(Math::abs(p_value), p_step))));
	const double scale = Math::pow(10.0, decimals);
	p_value *= scale;
	p_step *= scale;
	String step_str = String::num(p_step);
	String value_str = String::num(p_value);
	R128 step_r128;
	R128 value_r128;
	const R128 half_r128 = R128(0.5);
	r128FromString(&step_r128, step_str.ascii().get_data(), nullptr);
	r128FromString(&value_r128, value_str.ascii().get_data(), nullptr);
	r128Div(&value_r128, &value_r128, &step_r128);
	r128Add(&value_r128, &value_r128, &half_r128);
	r128Floor(&value_r128, &value_r128);
	r128Mul(&value_r128, &value_r128, &step_r128);
	if (scale != 1.0) {
		const R128 scale_r128 = R128(scale);
		r128Div(&value_r128, &value_r128, &scale_r128);
	}
	return (double)value_r128;
}

double NumericRange::calc_value(double p_val, double p_step) const
{
	if (Math::is_nan(p_val)) {
		return p_val;
	}

	if (p_step > 0) {
		if (Math::abs(min) > p_step * 1e14) {
			p_val = snapped_r128(p_val, p_step);
		}
		else {
			p_val = snapped_r128(p_val - min, p_step) + min;
		}
	}

	if (rounded_values) {
		p_val = Math::round(p_val);
	}

	if (!allow_greater && p_val > max - page) {
		p_val = max - page;
	}

	if (!allow_lesser && p_val < min) {
		p_val = min;
	}
	return p_val;
}

bool NumericRange::set_value(double p_val)
{
	double prev_val = value;
	value = calc_value(p_val, step);
	return value != prev_val && !(Math::is_nan(value) && Math::is_nan(prev_val));
}

bool NumericRange::set_min(double p_min)
{
	if (min == p_min) {
		return false;
	}
	min = p_min;
	max = MAX(max, min);
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_max(double p_max)
{
	double max_validated = MAX(p_max, min);
	if (max == max_validated) {
		return false;
	}
	max = max_validated;
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_step(double p_step)
{
	if (step == p_step) {
		return false;
	}
	step = p_step;
	return true;
}

bool NumericRange::set_page(double p_page)
{
	double page_validated = CLAMP(p_page, 0.0, max - min);
	if (page == page_validated) {
		return false;
	}
	page = page_validated;
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_as_ratio(double p_ratio)
{
	double v;
	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		v = Math::pow(2.0, exp_min + (exp_max - exp_min) * p_ratio);
	}
	else {
		double percent = (max - min) * p_ratio;
		if (step > 0) {
			double steps = std::round(percent / step);
			v = steps * step + min;
		}
		else {
			v = percent + min;
		}
	}
	v = CLAMP(v, min, max);
	return set_value(v);
}

double NumericRange::get_as_ratio() const
{
	if (Math::is_equal_approx(max, min)) {
		return 1.0;
	}

	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		double clamped = CLAMP(value, min, max);
		double v = Math::log(clamped) / Math::log(2.0);
		return CLAMP((v - exp_min) / (exp_max - exp_min), 0.0, 1.0);
	}
	else {
		double clamped = CLAMP(value, min, max);
		return CLAMP((clamped - min) / (max - min), 0.0, 1.0);
	}
}

void NumericRange::append_configuration_warnings(PackedStringArray& r_warnings) const
{
	if (exp_ratio && min < 0) {
		r_warnings.push_back(
			RTR("If \"Exp Edit\" is enabled, \"Min Value\" must be greater or equal to 0."));
	}
}

// --- progress_bar.cpp ---

Size2 ProgressBar::get_minimum_size() const
{
	Size2 minimum_size = theme_cache.background_style->get_minimum_size();
	minimum_size = minimum_size.max(theme_cache.fill_style->get_minimum_size());
	if (show_percentage) {
		String txt = "100%";
		TextLine tl = TextLine(txt, theme_cache.font, theme_cache.font_size);
		minimum_size.height = MAX(minimum_size.height,
			theme_cache.background_style->get_minimum_size().height + tl.get_size().y);
	}
	else { // this is needed, else the progressbar will collapse
		minimum_size = minimum_size.maxf(1);
	}
	return minimum_size;
}

int ProgressBar::get_fill_mode() { return mode; }

bool ProgressBar::is_percentage_shown() const { return show_percentage; }

bool ProgressBar::is_indeterminate() const { return indeterminate; }

bool ProgressBar::is_editor_preview_indeterminate_enabled() const
{
	return editor_preview_indeterminate;
}

ProgressBar::ProgressBar()
{
	set_v_size_flags(0);
	set_step(0.01);
}

// --- texture_progress_bar.cpp ---

void TextureProgressBar::set_under_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&under, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_under_texture() const { return under; }

void TextureProgressBar::set_over_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&over, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_over_texture() const { return over; }

int TextureProgressBar::get_stretch_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return stretch_margin[p_side];
}

bool TextureProgressBar::get_nine_patch_stretch() const { return nine_patch_stretch; }

Size2 TextureProgressBar::get_minimum_size() const
{
	if (nine_patch_stretch) {
		return Size2(stretch_margin[SIDE_LEFT] + stretch_margin[SIDE_RIGHT],
			stretch_margin[SIDE_TOP] + stretch_margin[SIDE_BOTTOM]);
	}

	Size2 size = Size2(1, 1);
	if (under.is_valid()) {
		size = size.max(under->get_size());
	}
	if (progress.is_valid()) {
		size = size.max(progress->get_size());
	}
	if (over.is_valid()) {
		size = size.max(over->get_size());
	}
	return size;
}

void TextureProgressBar::set_progress_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&progress, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_progress_texture() const { return progress; }

Point2 TextureProgressBar::get_progress_offset() const { return progress_offset; }

Color TextureProgressBar::get_tint_under() const { return tint_under; }

Color TextureProgressBar::get_tint_progress() const { return tint_progress; }

Color TextureProgressBar::get_tint_over() const { return tint_over; }

Point2 TextureProgressBar::unit_val_to_uv(float val)
{
	if (progress.is_null()) {
		return Point2();
	}

	if (val < 0) {
		val += 1;
	}
	if (val > 1) {
		val -= 1;
	}

	Point2 p = get_relative_center();

	// Minimal version of Liang-Barsky clipping algorithm
	float angle = (val * Math::TAU) - Math::PI * 0.5;
	Point2 dir = Vector2(Math::cos(angle), Math::sin(angle));
	float t1 = 1.0;
	float cp = 0.0;
	float cq = 0.0;
	float cr = 0.0;
	float edgeLeft = 0.0;
	float edgeRight = 1.0;
	float edgeBottom = 0.0;
	float edgeTop = 1.0;

	for (int edge = 0; edge < 4; edge++) {
		if (edge == 0) {
			if (dir.x > 0) {
				continue;
			}
			cq = -(edgeLeft - p.x);
			dir.x *= 2.0 * cq;
			cp = -dir.x;
		}
		else if (edge == 1) {
			if (dir.x < 0) {
				continue;
			}
			cq = (edgeRight - p.x);
			dir.x *= 2.0 * cq;
			cp = dir.x;
		}
		else if (edge == 2) {
			if (dir.y > 0) {
				continue;
			}
			cq = -(edgeBottom - p.y);
			dir.y *= 2.0 * cq;
			cp = -dir.y;
		}
		else if (edge == 3) {
			if (dir.y < 0) {
				continue;
			}
			cq = (edgeTop - p.y);
			dir.y *= 2.0 * cq;
			cp = dir.y;
		}
		cr = cq / cp;
		if (cr >= 0 && cr < t1) {
			t1 = cr;
		}
	}
	return (p + t1 * dir);
}

Point2 TextureProgressBar::get_relative_center()
{
	if (progress.is_null()) {
		return Point2();
	}
	Point2 p = progress->get_size() / 2;
	p += rad_center_off;
	p.x /= progress->get_width();
	p.y /= progress->get_height();
	p = p.clampf(0, 1);
	return p;
}

void TextureProgressBar::draw_nine_patch_stretched(
	const Ref<Texture2D>& p_texture, FillMode p_mode, double p_ratio, const Color& p_modulate)
{
	Vector2 texture_size = p_texture->get_size();
	Vector2 topleft = Vector2(stretch_margin[SIDE_LEFT], stretch_margin[SIDE_TOP]);
	Vector2 bottomright = Vector2(stretch_margin[SIDE_RIGHT], stretch_margin[SIDE_BOTTOM]);

	Rect2 src_rect = Rect2(Point2(), texture_size);
	Rect2 dst_rect = Rect2(Point2(), get_size());

	if (p_ratio < 1.0) {
		// Drawing a partially-filled 9-patch is a little tricky -
		// texture is divided by 3 sections toward fill direction,
		// then middle section is stretching while the other two aren't.

		double width_total = 0.0;
		double width_texture = 0.0;
		double first_section_size = 0.0;
		double last_section_size = 0.0;
		switch (p_mode) {
		case FILL_LEFT_TO_RIGHT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			first_section_size = topleft.x;
			last_section_size = bottomright.x;
		} break;
		case FILL_RIGHT_TO_LEFT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			// In contrast to `FILL_LEFT_TO_RIGHT`, `first_section_size` and `last_section_size`
			// should switch value.
			first_section_size = bottomright.x;
			last_section_size = topleft.x;
		} break;
		case FILL_TOP_TO_BOTTOM: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			first_section_size = topleft.y;
			last_section_size = bottomright.y;
		} break;
		case FILL_BOTTOM_TO_TOP: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			// Similar to `FILL_RIGHT_TO_LEFT`.
			first_section_size = bottomright.y;
			last_section_size = topleft.y;
		} break;
		case FILL_BILINEAR_LEFT_AND_RIGHT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			first_section_size = topleft.x;
			last_section_size = bottomright.x;
		} break;
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			first_section_size = topleft.y;
			last_section_size = bottomright.y;
		} break;
		case FILL_CLOCKWISE:
		case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE:
		case FILL_COUNTER_CLOCKWISE: {
			// Those modes are circular, not relevant for nine patch.
		} break;
		case FILL_MODE_MAX:
			break;
		}

		double width_filled = width_total * p_ratio;
		double middle_section_size =
			MAX(0.0, width_texture - first_section_size - last_section_size);

		// Maximum middle texture size.
		double max_middle_texture_size = middle_section_size;

		// Maximum real middle texture size.
		double max_middle_real_size =
			MAX(0.0, width_total - (first_section_size + last_section_size));

		switch (p_mode) {
		case FILL_BILINEAR_LEFT_AND_RIGHT:
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			last_section_size = MAX(0.0, last_section_size - (width_total - width_filled) * 0.5);
			first_section_size = MAX(0.0, first_section_size - (width_total - width_filled) * 0.5);

			// When `width_filled` increases, `middle_section_size` only increases when either of
			// `first_section_size` and `last_section_size` is zero. Also, it should always be
			// smaller than or equal to `(width_total - (first_section_size + last_section_size))`.
			double real_middle_size = width_filled - first_section_size - last_section_size;
			middle_section_size *=
				MIN(max_middle_real_size, real_middle_size) / max_middle_real_size;

			width_texture =
				MIN(width_texture, first_section_size + middle_section_size + last_section_size);
		} break;
		case FILL_MODE_MAX:
			break;
		default: {
			middle_section_size *=
				MIN(1.0, (MAX(0.0, width_filled - first_section_size) /
							 MAX(1.0, width_total - first_section_size - last_section_size)));
			last_section_size = MAX(0.0, last_section_size - (width_total - width_filled));
			first_section_size = MIN(first_section_size, width_filled);
			width_texture =
				MIN(width_texture, first_section_size + middle_section_size + last_section_size);
		}
		}

		switch (p_mode) {
		case FILL_LEFT_TO_RIGHT: {
			src_rect.size.x = width_texture;
			dst_rect.size.x = width_filled;
			topleft.x = first_section_size;
			bottomright.x = last_section_size;
		} break;
		case FILL_RIGHT_TO_LEFT: {
			src_rect.position.x += src_rect.size.x - width_texture;
			src_rect.size.x = width_texture;
			dst_rect.position.x += width_total - width_filled;
			dst_rect.size.x = width_filled;
			topleft.x = last_section_size;
			bottomright.x = first_section_size;
		} break;
		case FILL_TOP_TO_BOTTOM: {
			src_rect.size.y = width_texture;
			dst_rect.size.y = width_filled;
			bottomright.y = last_section_size;
			topleft.y = first_section_size;
		} break;
		case FILL_BOTTOM_TO_TOP: {
			src_rect.position.y += src_rect.size.y - width_texture;
			src_rect.size.y = width_texture;
			dst_rect.position.y += width_total - width_filled;
			dst_rect.size.y = width_filled;
			topleft.y = last_section_size;
			bottomright.y = first_section_size;
		} break;
		case FILL_BILINEAR_LEFT_AND_RIGHT: {
			double center_mapped_from_real_width =
				(width_total * 0.5 - topleft.x) / max_middle_real_size * max_middle_texture_size +
				topleft.x;
			double drift_from_unscaled_center = 0;
			if (bottomright.y != topleft.y) { // To avoid division by zero.
				drift_from_unscaled_center =
					(src_rect.size.x * 0.5 - center_mapped_from_real_width) *
					(last_section_size - first_section_size) / (bottomright.x - topleft.x);
			}

			src_rect.position.x +=
				center_mapped_from_real_width + drift_from_unscaled_center - width_texture * 0.5;
			src_rect.size.x = width_texture;
			dst_rect.position.x += (width_total - width_filled) * 0.5;
			dst_rect.size.x = width_filled;
			topleft.x = first_section_size;
			bottomright.x = last_section_size;
		} break;
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			double center_mapped_from_real_width =
				(width_total * 0.5 - topleft.y) / max_middle_real_size * max_middle_texture_size +
				topleft.y;
			double drift_from_unscaled_center = 0;
			if (bottomright.y != topleft.y) { // To avoid division by zero.
				drift_from_unscaled_center =
					(src_rect.size.y * 0.5 - center_mapped_from_real_width) *
					(last_section_size - first_section_size) / (bottomright.y - topleft.y);
			}

			src_rect.position.y +=
				center_mapped_from_real_width + drift_from_unscaled_center - width_texture * 0.5;
			src_rect.size.y = width_texture;
			dst_rect.position.y += (width_total - width_filled) * 0.5;
			dst_rect.size.y = width_filled;
			topleft.y = first_section_size;
			bottomright.y = last_section_size;
		} break;
		case FILL_CLOCKWISE:
		case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE:
		case FILL_COUNTER_CLOCKWISE: {
			// Those modes are circular, not relevant for nine patch.
		} break;
		case FILL_MODE_MAX:
			break;
		}
	}

	if (p_texture == progress) {
		dst_rect.position += progress_offset;
	}
	p_texture->get_rect_region(dst_rect, src_rect, dst_rect, src_rect);

	RID ci = get_canvas_item();
	Renderer::canvas_item_add_nine_patch(ci, dst_rect, src_rect,
		p_texture->get_scaled_rid(), topleft, bottomright, RSE::NINE_PATCH_STRETCH,
		RSE::NINE_PATCH_STRETCH, true, p_modulate);
}

void TextureProgressBar::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_PROGRESS_INDICATOR);
	} break;

	case NOTIFICATION_DRAW: {
		if (under.is_valid()) {
			if (nine_patch_stretch) {
				draw_nine_patch_stretched(under, mode, 1.0, tint_under);
			}
			else {
				draw_texture(under.ptr(), Point2(), tint_under);
			}
		}

		if (progress.is_valid()) {
			const bool is_radial_mode = (mode == FILL_CLOCKWISE || mode == FILL_COUNTER_CLOCKWISE ||
										 mode == FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE);

			if (nine_patch_stretch && !is_radial_mode) {
				draw_nine_patch_stretched(progress, mode, get_as_ratio(), tint_progress);
			}
			else {
				Size2 s = progress->get_size();
				switch (mode) {
				case FILL_LEFT_TO_RIGHT: {
					Rect2 region = Rect2(progress_offset, Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(Point2(), Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_RIGHT_TO_LEFT: {
					Rect2 region = Rect2(progress_offset + Point2(s.x - s.x * get_as_ratio(), 0),
						Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(
						Point2(s.x - s.x * get_as_ratio(), 0), Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_TOP_TO_BOTTOM: {
					Rect2 region =
						Rect2(progress_offset + Point2(), Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(Point2(), Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_BOTTOM_TO_TOP: {
					Rect2 region = Rect2(progress_offset + Point2(0, s.y - s.y * get_as_ratio()),
						Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(
						Point2(0, s.y - s.y * get_as_ratio()), Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_CLOCKWISE:
				case FILL_COUNTER_CLOCKWISE:
				case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE: {
					if (nine_patch_stretch) {
						s = get_size();
					}

					float val = get_as_ratio() * rad_max_degrees / 360;
					if (val == 1) {
						Rect2 region = Rect2(progress_offset, s);
						Rect2 source = Rect2(Point2(), progress->get_size());
						draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
					}
					else if (val != 0) {
						LocalVector<float> pts;
						float direction = mode == FILL_COUNTER_CLOCKWISE ? -1 : 1;
						float start;

						if (mode == FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE) {
							start = rad_init_angle / 360 - val / 2;
						}
						else {
							start = rad_init_angle / 360;
						}

						float end = start + direction * val;
						float from = MIN(start, end);
						float to = MAX(start, end);
						pts.push_back(from);
						for (float corner = Math::floor(from * 4 + 0.5) * 0.25 + 0.125; corner < to;
							 corner += 0.25) {
							pts.push_back(corner);
						}
						pts.push_back(to);

						Vector<Point2> uvs;
						Vector<Point2> points;
						for (const float& f : pts) {
							Point2 uv = unit_val_to_uv(f);
							if (uvs.has(uv)) {
								continue;
							}
							points.push_back(progress_offset + Point2(uv.x * s.x, uv.y * s.y));
							uvs.push_back(uv);
						}

						// Filter out an edge case where almost equal `from`, `to` were mapped to
						// the same UV.
						if (points.size() >= 2) {
							Point2 center_point = get_relative_center();
							points.push_back(progress_offset + s * center_point);
							uvs.push_back(center_point);

							Vector<Color> colors;
							colors.push_back(tint_progress);
							draw_polygon(points, colors, uvs, progress);
						}
					}
				} break;
				case FILL_BILINEAR_LEFT_AND_RIGHT: {
					Rect2 region =
						Rect2(progress_offset + Point2(s.x / 2 - s.x * get_as_ratio() / 2, 0),
							Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(Point2(s.x / 2 - s.x * get_as_ratio() / 2, 0),
						Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_BILINEAR_TOP_AND_BOTTOM: {
					Rect2 region =
						Rect2(progress_offset + Point2(0, s.y / 2 - s.y * get_as_ratio() / 2),
							Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(Point2(0, s.y / 2 - s.y * get_as_ratio() / 2),
						Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_MODE_MAX:
					break;
				}
			}
#ifdef TOOLS_ENABLED
			// Draw a reference cross for radial modes.
			if (is_radial_mode && is_part_of_edited_scene()) {
				Point2 p;

				if (nine_patch_stretch) {
					p = get_size();
				}
				else {
					p = progress->get_size();
				}

				p *= get_relative_center();
				p += progress_offset;
				draw_line(p - Point2(8, 0), p + Point2(8, 0), Color(0.9, 0.5, 0.5), 2);
				draw_line(p - Point2(0, 8), p + Point2(0, 8), Color(0.9, 0.5, 0.5), 2);
			}
#endif
		}

		if (over.is_valid()) {
			if (nine_patch_stretch) {
				draw_nine_patch_stretched(over, mode, 1.0, tint_over);
			}
			else {
				draw_texture(over.ptr(), Point2(), tint_over);
			}
		}
	} break;
	}
}

int TextureProgressBar::get_fill_mode() { return mode; }

float TextureProgressBar::get_radial_initial_angle() { return rad_init_angle; }

float TextureProgressBar::get_fill_degrees() { return rad_max_degrees; }

Point2 TextureProgressBar::get_radial_center_offset() { return rad_center_off; }

TextureProgressBar::TextureProgressBar() { set_mouse_filter(MOUSE_FILTER_PASS); }



void TextureProgressBar::_set_texture(Ref<Texture2D>*, Ref<Texture2D> const&) {}

void TextureProgressBar::set_tint_under(Color const&) {}

void TextureProgressBar::set_fill_mode(int) {}

// --- slider.cpp ---

Size2 Slider::get_minimum_size() const
{
	Size2i ss = theme_cache.slider_style->get_minimum_size();
	Size2i rs = theme_cache.grabber_icon->get_size();

	if (orientation == HORIZONTAL) {
		return Size2i(ss.width, MAX(ss.height, rs.height));
	}
	else {
		return Size2i(MAX(ss.width, rs.width), ss.height);
	}
}

void Slider::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (Input::is_action_just_released("ui_left") ||
			Input::is_action_just_released("ui_right") || Input::is_action_just_released("ui_up") ||
			Input::is_action_just_released("ui_down")) {
			gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;
			set_process_internal(false);
			return;
		}

		gamepad_event_delay_ms -= get_process_delta_time();
		if (gamepad_event_delay_ms <= 0) {
			gamepad_event_delay_ms = GAMEPAD_EVENT_REPEAT_RATE_MS + gamepad_event_delay_ms;
			if (orientation == HORIZONTAL) {
				if (Input::is_action_pressed("ui_left")) {
					if (is_layout_rtl()) {
						set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
					}
					else {
						set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
					}
				}

				if (Input::is_action_pressed("ui_right")) {
					if (is_layout_rtl()) {
						set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
					}
					else {
						set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
					}
				}
			}
			else if (orientation == VERTICAL) {
				if (Input::is_action_pressed("ui_down")) {
					set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
				}

				if (Input::is_action_pressed("ui_up")) {
					set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
				}
			}
		}
	} break;

	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_SLIDER);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_EXIT_TREE: {
		mouse_inside = false;
		grab.active = false;
	} break;

	case NOTIFICATION_DRAW: {
		RID ci = get_canvas_item();
		Size2i size = get_size();
		double ratio = Math::is_nan(get_as_ratio()) ? 0 : get_as_ratio();

		Ref<StyleBox> style = theme_cache.slider_style;
		Ref<Texture2D> tick = theme_cache.tick_icon;

		bool highlighted = editable && (mouse_inside || has_focus(true));
		Ref<Texture2D> grabber;
		if (editable) {
			if (highlighted) {
				grabber = theme_cache.grabber_hl_icon;
			}
			else {
				grabber = theme_cache.grabber_icon;
			}
		}
		else {
			grabber = theme_cache.grabber_disabled_icon;
		}

		Ref<StyleBox> grabber_area;
		if (highlighted) {
			grabber_area = theme_cache.grabber_area_hl_style;
		}
		else {
			grabber_area = theme_cache.grabber_area_style;
		}

		if (orientation == VERTICAL) {
			int widget_width = style->get_minimum_size().width;
			double areasize =
				size.height - (theme_cache.center_grabber ? 0 : grabber->get_height());
			int grabber_shift = theme_cache.center_grabber ? grabber->get_height() / 2 : 0;
			style->draw(ci, Rect2i(Point2i(size.width / 2 - widget_width / 2, 0),
								Size2i(widget_width, size.height)));
			grabber_area->draw(ci,
				Rect2i(Point2i((size.width - widget_width) / 2,
						   Math::round(size.height - areasize * ratio - grabber->get_height() / 2 +
									   grabber_shift)),
					Size2i(widget_width, Math::round(areasize * ratio + grabber->get_height() / 2 -
													 grabber_shift))));

			if (ticks > 1) {
				int grabber_offset = (grabber->get_height() / 2 - tick->get_height() / 2);
				for (int i = 0; i < ticks; i++) {
					if (!ticks_on_borders && (i == 0 || i + 1 == ticks)) {
						continue;
					}
					int ofs = (i * areasize / (ticks - 1)) + grabber_offset - grabber_shift;

					if (ticks_position == TICK_POSITION_BOTTOM_RIGHT ||
						ticks_position == TICK_POSITION_BOTH) {
						tick->draw(ci, Point2i(widget_width + (size.width - widget_width) / 2 +
												   theme_cache.tick_offset,
										   ofs));
					}

					if (ticks_position == TICK_POSITION_TOP_LEFT ||
						ticks_position == TICK_POSITION_BOTH) {
						Point2i pos = Point2i((size.width - widget_width) / 2 - tick->get_width() -
												  theme_cache.tick_offset,
							ofs);
						tick->draw_rect(
							ci, Rect2i(pos, Size2i(-tick->get_width(), tick->get_height())));
					}

					if (ticks_position == TICK_POSITION_CENTER) {
						tick->draw(ci,
							Point2i((size.width - tick->get_width()) / 2 + theme_cache.tick_offset,
								ofs));
					}
				}
			}
			grabber->draw(
				ci, Point2i(size.width / 2 - grabber->get_width() / 2 + theme_cache.grabber_offset,
						size.height - ratio * areasize - grabber->get_height() + grabber_shift));
		}
		else {
			int widget_height = style->get_minimum_size().height;
			double areasize =
				size.width - (theme_cache.center_grabber ? 0 : grabber->get_size().width);
			int grabber_shift = theme_cache.center_grabber ? -grabber->get_width() / 2 : 0;
			bool rtl = is_layout_rtl();

			style->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2),
								Size2i(size.width, widget_height)));
			int p = areasize * (rtl ? 1 - ratio : ratio) + grabber->get_width() / 2 + grabber_shift;
			if (rtl) {
				grabber_area->draw(ci, Rect2i(Point2i(p, (size.height - widget_height) / 2),
										   Size2i(size.width - p, widget_height)));
			}
			else {
				grabber_area->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2),
										   Size2i(p, widget_height)));
			}

			if (ticks > 1) {
				int grabber_offset = (grabber->get_width() / 2 - tick->get_width() / 2);
				for (int i = 0; i < ticks; i++) {
					if ((!ticks_on_borders) && ((i == 0) || ((i + 1) == ticks))) {
						continue;
					}
					int ofs = (i * areasize / (ticks - 1)) + grabber_offset + grabber_shift;

					if (ticks_position == TICK_POSITION_BOTTOM_RIGHT ||
						ticks_position == TICK_POSITION_BOTH) {
						tick->draw(
							ci, Point2i(ofs, widget_height + (size.height - widget_height) / 2 +
												 theme_cache.tick_offset));
					}

					if (ticks_position == TICK_POSITION_TOP_LEFT ||
						ticks_position == TICK_POSITION_BOTH) {
						Point2i pos =
							Point2i(ofs, (size.height - widget_height) / 2 - tick->get_height() -
											 theme_cache.tick_offset);
						tick->draw_rect(
							ci, Rect2i(pos, Size2i(tick->get_width(), -tick->get_height())));
					}

					if (ticks_position == TICK_POSITION_CENTER) {
						tick->draw(ci, Point2i(ofs, (size.height - tick->get_height()) / 2 +
														theme_cache.tick_offset));
					}
				}
			}
			grabber->draw(
				ci, Point2i((rtl ? 1 - ratio : ratio) * areasize + grabber_shift,
						size.height / 2 - grabber->get_height() / 2 + theme_cache.grabber_offset));
		}
	} break;
	}
}

void Slider::set_custom_step(double p_custom_step) { custom_step = p_custom_step; }

double Slider::get_custom_step() const { return custom_step; }

int Slider::get_ticks() const { return ticks; }

bool Slider::get_ticks_on_borders() const { return ticks_on_borders; }

Slider::TickPosition Slider::get_ticks_position() const { return ticks_position; }

bool Slider::is_editable() const { return editable; }

void Slider::set_scrollable(bool p_scrollable) { scrollable = p_scrollable; }

bool Slider::is_scrollable() const { return scrollable; }

Slider::Slider(Orientation p_orientation)
{
	orientation = p_orientation;
	if (orientation == HORIZONTAL) {
		set_v_size_flags(0);
	}
	else {
		set_h_size_flags(0);
	}
	set_focus_mode(FOCUS_ALL);
}

// --- spin_box.cpp ---

Size2 SpinBox::get_minimum_size() const
{
	Size2 ms = line_edit->get_bound_minimum_size();
	ms.width += sizing_cache.buttons_block_width;
	return ms;
}

LineEdit* SpinBox::get_line_edit() { return line_edit; }

void SpinBox::_line_edit_input(const InputEvent& p_event)
{
	if (drag.enabled) {
		line_edit->accept_event();
	}
}

void SpinBox::_release_mouse_from_drag_mode()
{
	if (drag.enabled) {
		drag.enabled = false;
		Input::set_mouse_mode(Input::MouseMode::MOUSE_MODE_HIDDEN);
		warp_mouse(drag.capture_pos);
		Input::set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
	}
}

void SpinBox::_arrow_clicked(bool p_up)
{
	double arrow_step = get_custom_arrow_step() != 0.0 ? get_custom_arrow_step() : get_step();
	if (custom_arrow_round) {
		// Arrow button is being pressed, snap the value to next `arrow_step`.
		// `arrow_step` should be a multiple of `step`, otherwise it may not be able to
		// increase/decrease the value.
		arrow_step = Math::snapped(arrow_step, get_step());
		double new_value = range.calc_value(get_value(), arrow_step);
		if ((p_up && new_value <= get_value()) || (!p_up && new_value >= get_value())) {
			new_value = range.calc_value(get_value() + (p_up ? arrow_step : -arrow_step), arrow_step);
		}
		set_value(new_value);
	}
	else {
		set_value(get_value() + (p_up ? arrow_step : -arrow_step));
	}
}

void SpinBox::_line_edit_editing_toggled(bool p_toggled_on)
{
	if (p_toggled_on) {
		int col = line_edit->get_caret_column();
		_update_text();
		line_edit->set_caret_column(col);

		// LineEdit text might change and it clears any selection. Have to re-select here.
		if (line_edit->is_select_all_on_focus() &&
			!Input::is_mouse_button_pressed(MouseButton::LEFT)) {
			line_edit->select_all();
		}
	}
	else {
		accepted = true;

		if (Input::is_action_pressed("ui_cancel") ||
			line_edit->get_text().is_empty()) {
			_update_text(); // Revert text if editing was canceled.
		}
		else {
			line_edit->set_text(line_edit->get_text().trim_suffix(".").trim_suffix(","));
			_update_text(
				true); // Update text in case value was changed this frame (e.g. on `focus_exited`).
			_text_submitted(line_edit->get_text());
		}
	}
}

inline void SpinBox::_compute_sizes()
{
	int buttons_block_wanted_width =
		theme_cache.buttons_width + theme_cache.field_and_buttons_separation;
	int buttons_block_icon_enforced_width =
		_get_widest_button_icon_width() + theme_cache.field_and_buttons_separation;

#ifndef DISABLE_DEPRECATED
	const bool min_width_from_icons =
		theme_cache.set_min_buttons_width_from_icons || (theme_cache.buttons_width < 0);
#else
	const bool min_width_from_icons = theme_cache.buttons_width < 0;
#endif
	int w = min_width_from_icons != 0
				? MAX(buttons_block_icon_enforced_width, buttons_block_wanted_width)
				: buttons_block_wanted_width;

	if (w != sizing_cache.buttons_block_width) {
		line_edit->set_offset(SIDE_LEFT, 0);
		line_edit->set_offset(SIDE_RIGHT, -w);
		sizing_cache.buttons_block_width = w;
	}

	Size2i size = get_size();

	sizing_cache.buttons_width = w - theme_cache.field_and_buttons_separation;
	sizing_cache.buttons_vertical_separation =
		CLAMP(theme_cache.buttons_vertical_separation, 0, size.height);
	sizing_cache.buttons_left = is_layout_rtl() ? 0 : size.width - sizing_cache.buttons_width;
	sizing_cache.button_up_height = (size.height - sizing_cache.buttons_vertical_separation) / 2;
	sizing_cache.button_down_height =
		size.height - sizing_cache.button_up_height - sizing_cache.buttons_vertical_separation;
	sizing_cache.second_button_top = size.height - sizing_cache.button_down_height;

	sizing_cache.buttons_separator_top = sizing_cache.button_up_height;
	sizing_cache.field_and_buttons_separator_left =
		is_layout_rtl() ? sizing_cache.buttons_width
						: size.width - sizing_cache.buttons_block_width;
	sizing_cache.field_and_buttons_separator_width = theme_cache.field_and_buttons_separation;
}

inline int SpinBox::_get_widest_button_icon_width()
{
	int max = 0;
#ifndef DISABLE_DEPRECATED
	max = MAX(max, theme_cache.updown_icon->get_width());
#endif
	max = MAX(max, theme_cache.up_icon->get_width());
	max = MAX(max, theme_cache.up_hover_icon->get_width());
	max = MAX(max, theme_cache.up_pressed_icon->get_width());
	max = MAX(max, theme_cache.up_disabled_icon->get_width());
	max = MAX(max, theme_cache.down_icon->get_width());
	max = MAX(max, theme_cache.down_hover_icon->get_width());
	max = MAX(max, theme_cache.down_pressed_icon->get_width());
	max = MAX(max, theme_cache.down_disabled_icon->get_width());
	return max;
}

void SpinBox::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	line_edit->set_horizontal_alignment(p_alignment);
}

HorizontalAlignment SpinBox::get_horizontal_alignment() const
{
	return line_edit->get_horizontal_alignment();
}

void SpinBox::set_suffix(const String& p_suffix)
{
	if (suffix == p_suffix) {
		return;
	}

	suffix = p_suffix;
	_update_text();
}

String SpinBox::get_suffix() const { return suffix; }

void SpinBox::set_prefix(const String& p_prefix)
{
	if (prefix == p_prefix) {
		return;
	}

	prefix = p_prefix;
	_update_text();
}

String SpinBox::get_prefix() const { return prefix; }

bool SpinBox::get_update_on_text_changed() const { return update_on_text_changed; }

void SpinBox::set_select_all_on_focus(bool p_enabled)
{
	line_edit->set_select_all_on_focus(p_enabled);
}

bool SpinBox::is_select_all_on_focus() const { return line_edit->is_select_all_on_focus(); }

bool SpinBox::is_editable() const { return line_edit->is_editable(); }

void SpinBox::apply() { _text_submitted(line_edit->get_text()); }

void SpinBox::set_custom_arrow_step(double p_custom_arrow_step)
{
	custom_arrow_step = p_custom_arrow_step;
}

double SpinBox::get_custom_arrow_step() const { return custom_arrow_step; }

void SpinBox::set_custom_arrow_round(bool p_round) { custom_arrow_round = p_round; }

bool SpinBox::is_custom_arrow_rounding() const { return custom_arrow_round; }

void SpinBox::_value_changed(double p_value)
{
	_update_buttons_state_for_current_value();
}

void SpinBox::_update_text(bool p_only_update_if_value_changed) {}



void SpinBox::_text_submitted(String const&) {}

void SpinBox::set_editable(bool) {}

void SpinBox::_update_buttons_state_for_current_value() {}

// --- scroll_bar.cpp ---

bool ScrollBar::focus_by_default = false;

void ScrollBar::set_can_focus_by_default(bool p_can_focus) { focus_by_default = p_can_focus; }

double ScrollBar::get_grabber_min_size() const
{
	Ref<StyleBox> grabber = theme_cache.grabber_style;
	Size2 gminsize = grabber->get_minimum_size();
	return (orientation == VERTICAL) ? gminsize.height : gminsize.width;
}

double ScrollBar::get_grabber_size() const
{
	float range = get_max() - get_min();
	if (range <= 0) {
		return 0;
	}

	float page = (get_page() > 0) ? get_page() : 0;
	double area_size = get_area_size();
	double grabber_size = page / range * area_size;
	return grabber_size + get_grabber_min_size();
}

double ScrollBar::get_area_size() const
{
	switch (orientation) {
	case VERTICAL: {
		double area = get_size().height;
		area -= theme_cache.scroll_style->get_minimum_size().height;
		area -= theme_cache.increment_icon->get_height();
		area -= theme_cache.decrement_icon->get_height();
		area -= get_grabber_min_size();
		return area;
	} break;
	case HORIZONTAL: {
		double area = get_size().width;
		area -= theme_cache.scroll_style->get_minimum_size().width;
		area -= theme_cache.increment_icon->get_width();
		area -= theme_cache.decrement_icon->get_width();
		area -= get_grabber_min_size();
		return area;
	} break;
	default: {
		return 0.0;
	}
	}
}

double ScrollBar::get_grabber_offset() const { return get_area_size() * get_as_ratio(); }

Size2 ScrollBar::get_minimum_size() const
{
	Ref<Texture2D> incr = theme_cache.increment_icon;
	Ref<Texture2D> decr = theme_cache.decrement_icon;
	Ref<StyleBox> bg = theme_cache.scroll_style;
	Size2 minsize;

	if (orientation == VERTICAL) {
		int padding_left = MAX(theme_cache.padding_left, 0);
		int padding_right = MAX(theme_cache.padding_right, 0);
		minsize.width = MAX(incr->get_size().width, bg->get_minimum_size().width);
		minsize.width += padding_left + padding_right;
		minsize.height += incr->get_size().height;
		minsize.height += decr->get_size().height;
		minsize.height += bg->get_minimum_size().height;
		minsize.height += get_grabber_min_size();
	}

	if (orientation == HORIZONTAL) {
		int padding_top = MAX(theme_cache.padding_top, 0);
		int padding_bottom = MAX(theme_cache.padding_bottom, 0);
		minsize.height = MAX(incr->get_size().height, bg->get_minimum_size().height);
		minsize.height += padding_top + padding_bottom;
		minsize.width += incr->get_size().width;
		minsize.width += decr->get_size().width;
		minsize.width += bg->get_minimum_size().width;
		minsize.width += get_grabber_min_size();
	}

	return minsize;
}

void ScrollBar::scroll(double p_amount) { scroll_to(get_value() + p_amount); }

void ScrollBar::scroll_to(double p_position) {}

void ScrollBar::set_custom_step(float p_custom_step) { custom_step = p_custom_step; }

float ScrollBar::get_custom_step() const { return custom_step; }

void ScrollBar::_drag_node_input(const InputEvent& p_input)
{
	if (!drag_node_enabled) {
		return;
	}

	if (p_input.type == InputEventType::MOUSE_BUTTON) {
		if (p_input.mouse_button.button_index != MouseButton::LEFT) {
			return;
		}

		if (p_input.is_pressed()) {
			drag_node_speed = Vector2();
			drag_node_accum = Vector2();
			last_drag_node_accum = Vector2();
			drag_node_from = Vector2(orientation == HORIZONTAL ? get_value() : 0,
				orientation == VERTICAL ? get_value() : 0);
			drag_node_touching = DisplayServer::is_touchscreen_available();
			drag_node_touching_deaccel = false;
			time_since_motion = 0;

			if (drag_node_touching) {
				set_process_internal(true);
				time_since_motion = 0;
			}
		}
		else if (drag_node_touching) {
			if (drag_node_speed == Vector2()) {
				drag_node_touching_deaccel = false;
				drag_node_touching = false;
				set_process_internal(false);
			}
			else {
				drag_node_touching_deaccel = true;
			}
		}
	}

	if (p_input.type == InputEventType::MOUSE_MOTION) {
		if (drag_node_touching && !drag_node_touching_deaccel) {
			Vector2 motion = p_input.mouse_motion.relative;

			drag_node_accum -= motion;
			Vector2 diff = drag_node_from + drag_node_accum;

			if (orientation == HORIZONTAL) {
				scroll_to(diff.x);
			}

			if (orientation == VERTICAL) {
				scroll_to(diff.y);
			}

			time_since_motion = 0;
		}
	}
}

NodePath ScrollBar::get_drag_node() const { return drag_node_path; }

void ScrollBar::set_drag_node_enabled(bool p_enable) { drag_node_enabled = p_enable; }

void ScrollBar::set_smooth_scroll_enabled(bool p_enable) { smooth_scroll_enabled = p_enable; }

bool ScrollBar::is_smooth_scroll_enabled() const { return smooth_scroll_enabled; }

ScrollBar::ScrollBar(Orientation p_orientation)
{
	orientation = p_orientation;
	if (orientation == HORIZONTAL) {
		set_v_size_flags(0);
	}
	else {
		set_h_size_flags(0);
	}

	if (focus_by_default) {
		set_focus_mode(FOCUS_ALL);
	}
	else {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
	set_step(0);
}

ScrollBar::~ScrollBar() {}
