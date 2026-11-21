/**************************************************************************/
/*  button.cpp                                                            */
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

#include "button.h"
#include "core/os/os.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "menu_button.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "option_button.h"
#include "menu_bar.h"

ButtonBehavior::DrawMode ButtonBehavior::get_draw_mode() const
{
	if (disabled) {
		return DRAW_DISABLED;
	}

	if (in_shortcut_feedback) {
		return DRAW_HOVER_PRESSED;
	}

	if (!press_attempt && hovering) {
		if (pressed) {
			return DRAW_HOVER_PRESSED;
		}
		return DRAW_HOVER;
	}
	else {
		bool pressing;
		if (press_attempt) {
			pressing = (pressing_inside || keep_pressed_outside);
			if (pressed) {
				pressing = !pressing;
			}
		}
		else {
			pressing = pressed;
		}

		return pressing ? DRAW_PRESSED : DRAW_NORMAL;
	}
}

void Button::set_style(Style p_style)
{
	if (style == p_style) {
		return;
	}
	style = p_style;
	if (style == STYLE_CHECK_BOX || style == STYLE_CHECK_BUTTON) {
		set_toggle_mode(true);
	}
	else if (style == STYLE_LINK) {
		if (link_text_buf.is_null()) {
			link_text_buf.instantiate();
		}
		set_focus_mode(FOCUS_ACCESSIBILITY);
		set_default_cursor_shape(CURSOR_POINTING_HAND);
	}
	update_minimum_size();
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_pressed(bool p_pressed)
{
	if (!behavior.toggle_mode) {
		return;
	}
	if (behavior.pressed == p_pressed) {
		return;
	}
	behavior.pressed = p_pressed;
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_pressed_no_signal(bool p_pressed)
{
	if (behavior.pressed == p_pressed) {
		return;
	}
	behavior.pressed = p_pressed;
	queue_redraw();
}

void Button::set_toggle_mode(bool p_on)
{
	if (!p_on) {
		set_pressed(false);
	}
	behavior.toggle_mode = p_on;
	queue_accessibility_update();
	update_configuration_warnings();
}

void Button::set_disabled(bool p_disabled)
{
	if (behavior.disabled == p_disabled) {
		return;
	}
	behavior.disabled = p_disabled;
	if (p_disabled) {
		if (!behavior.toggle_mode) {
			behavior.pressed = false;
		}
		behavior.press_attempt = false;
		behavior.pressing_inside = false;
	}
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_shortcut_in_tooltip(bool p_on)
{
	if (behavior.shortcut_in_tooltip != p_on) {
		behavior.shortcut_in_tooltip = p_on;
		queue_accessibility_update();
	}
}

void Button::set_shortcut(const Ref<Shortcut>& p_shortcut)
{
	if (behavior.shortcut != p_shortcut) {
		behavior.shortcut = p_shortcut;
		set_process_shortcut_input(behavior.shortcut.is_valid());
		queue_accessibility_update();
	}
}

void Button::set_button_group(const Ref<ButtonGroup>& p_group)
{
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.erase(this);
	}
	behavior.button_group = p_group;
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.insert(this);
	}
	queue_redraw();
	update_configuration_warnings();
}

void Button::pressed()
{
	if (style == STYLE_LINK && !uri.is_empty()) {
		OS::shell_open(uri);
	}
}

void Button::toggled(bool p_pressed) {}

void Button::set_uri(const String& p_uri)
{
	if (uri != p_uri) {
		uri = p_uri;
		queue_accessibility_update();
	}
}

void Button::set_swatch_color(const Color& p_color)
{
	swatch_color = p_color;
	queue_redraw();
	queue_accessibility_update();
}

bool Button::has_point(const Point2& p_point) const
{
	if (style == STYLE_TEXTURE && textures.click_mask.is_valid()) {
		Point2 point = p_point;
		Rect2 rect;
		Size2 mask_size = textures.click_mask->get_size();

		if (!textures.position_rect.has_area()) {
			rect.size = mask_size;
		}
		else if (textures.tile) {
			rect.size = mask_size;
			if (textures.position_rect.has_point(point)) {
				int cols = (int)Math::ceil(textures.position_rect.size.x / mask_size.x);
				int rows = (int)Math::ceil(textures.position_rect.size.y / mask_size.y);
				int col = (int)(point.x / mask_size.x) % cols;
				int row = (int)(point.y / mask_size.y) % rows;
				point.x -= mask_size.x * col;
				point.y -= mask_size.y * row;
			}
		}
		else {
			Point2 ofs = textures.position_rect.position;
			Size2 scale = mask_size / textures.position_rect.size;

			if (textures.stretch_mode == STRETCH_KEEP_ASPECT_COVERED) {
				float min = MIN(scale.x, scale.y);
				scale.x = min;
				scale.y = min;
				ofs -= textures.texture_region.position / min;
			}

			point -= ofs;
			point *= scale;

			rect.position = textures.texture_region.position.maxf(0);
			rect.size = mask_size.min(textures.texture_region.size);
		}

		if (!rect.has_point(point)) {
			return false;
		}

		Point2i p = point;
		return textures.click_mask->get_bitv(p);
	}

	ERR_READ_THREAD_GUARD_V(false);
	Rect2 rect = Rect2(Point2(), get_size()).grow(behavior.click_margin);
	return rect.has_area() && rect.has_point(p_point);
}

Control::CursorShape Button::get_cursor_shape(const Point2& p_pos) const
{
	if (style == STYLE_LINK) {
		return is_disabled() ? CURSOR_ARROW : get_default_cursor_shape();
	}
	return Control::get_cursor_shape(p_pos);
}

PackedStringArray Button::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (behavior.button_group.is_valid() && !behavior.toggle_mode) {
		warnings.push_back(RTR("ButtonGroup is intended to be used only with buttons that have "
							   "toggle_mode set to true."));
	}

	return warnings;
}

Size2 Button::_get_check_icon_size() const
{
	Size2 tex_size;
	if (style == STYLE_CHECK_BOX) {
		const Ref<Texture2D> candidates[] = {theme_cache.checked, theme_cache.unchecked,
			theme_cache.radio_checked, theme_cache.radio_unchecked, theme_cache.checked_disabled,
			theme_cache.unchecked_disabled, theme_cache.radio_checked_disabled,
			theme_cache.radio_unchecked_disabled};
		for (const Ref<Texture2D>& tex : candidates) {
			if (tex.is_valid()) {
				tex_size = tex_size.max(tex->get_size());
			}
		}
	}
	else if (style == STYLE_CHECK_BUTTON) {
		Ref<Texture2D> on_tex = is_layout_rtl()
									? (is_disabled() ? theme_cache.checked_disabled_mirrored
													 : theme_cache.checked_mirrored)
									: (is_disabled() ? theme_cache.checked_disabled
													 : theme_cache.checked);
		Ref<Texture2D> off_tex = is_layout_rtl()
									 ? (is_disabled() ? theme_cache.unchecked_disabled_mirrored
													  : theme_cache.unchecked_mirrored)
									 : (is_disabled() ? theme_cache.unchecked_disabled
													  : theme_cache.unchecked);
		if (on_tex.is_valid()) {
			tex_size = on_tex->get_size();
		}
		if (off_tex.is_valid()) {
			tex_size = tex_size.max(off_tex->get_size());
		}
	}
	return _fit_icon_size(tex_size);
}

Size2 Button::get_minimum_size() const
{
	if (style == STYLE_LINK) {
		Size2 minsize = link_text_buf.is_valid() ? link_text_buf->get_size() : Size2();
		if (overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			minsize.width = 0;
		}
		return minsize;
	}

	if (style == STYLE_TEXTURE) {
		Size2 rscale = Control::get_minimum_size();
		if (!textures.ignore_texture_size) {
			if (textures.normal.is_valid()) {
				rscale = textures.normal->get_size();
			}
			else if (textures.pressed.is_valid()) {
				rscale = textures.pressed->get_size();
			}
			else if (textures.hover.is_valid()) {
				rscale = textures.hover->get_size();
			}
			else if (textures.click_mask.is_valid()) {
				rscale = textures.click_mask->get_size();
			}
			else {
				rscale = Size2();
			}
		}
		return rscale.abs();
	}

	if (style == STYLE_COLOR_SWATCH) {
		return Control::get_minimum_size();
	}

	Ref<Texture2D> _icon = icon;
	if (_icon.is_null() && has_theme_icon(SNAME("icon"))) {
		_icon = theme_cache.icon;
	}

	Size2 minsize = get_minimum_size_for_text_and_icon("", _icon);

	if (style == STYLE_CHECK_BOX || style == STYLE_CHECK_BUTTON) {
		const Size2 tex_size = _get_check_icon_size();
		if (tex_size.width > 0 || tex_size.height > 0) {
			const Size2 padding = _get_largest_stylebox_size();
			Size2 content_size = minsize - padding;
			if (content_size.width > 0 && tex_size.width > 0) {
				content_size.width += MAX(0, theme_cache.h_separation);
			}
			content_size.width += tex_size.width;
			content_size.height = MAX(content_size.height, tex_size.height);
			minsize = content_size + padding;
		}
	}

	return minsize;
}

void Button::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		if (style == STYLE_CHECK_BOX) {
			AccessibilityServer::update_set_role(ae,
				is_radio() ? AccessibilityServerEnums::AccessibilityRole::ROLE_RADIO_BUTTON
						   : AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BOX);
		}
		else if (style == STYLE_CHECK_BUTTON) {
			AccessibilityServer::update_set_role(
				ae, AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BUTTON);
		}
		else {
			AccessibilityServer::update_set_role(
				ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
			if (style == STYLE_COLOR_SWATCH) {
				AccessibilityServer::update_set_color_value(ae, swatch_color);
			}
		}
	} break;

	case NOTIFICATION_THEME_CHANGED:
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (style == STYLE_CHECK_BOX) {
			const float w = _get_check_icon_size().width;
			_set_internal_margin(SIDE_LEFT, is_layout_rtl() ? 0.f : w);
			_set_internal_margin(SIDE_RIGHT, is_layout_rtl() ? w : 0.f);
		}
		else if (style == STYLE_CHECK_BUTTON) {
			const float w = _get_check_icon_size().width;
			_set_internal_margin(SIDE_LEFT, is_layout_rtl() ? w : 0.f);
			_set_internal_margin(SIDE_RIGHT, is_layout_rtl() ? 0.f : w);
		}
	} break;

	case NOTIFICATION_DRAW: {
		if (style == STYLE_TEXTURE) {
			DrawMode draw_mode = get_draw_mode();
			Ref<Texture2D> texdraw;

			switch (draw_mode) {
			case DRAW_NORMAL:
				texdraw = textures.normal;
				break;
			case DRAW_HOVER_PRESSED:
			case DRAW_PRESSED:
				texdraw = textures.pressed.is_valid()
							  ? textures.pressed
							  : (textures.hover.is_valid() ? textures.hover : textures.normal);
				break;
			case DRAW_HOVER:
				texdraw = textures.hover.is_valid()
							  ? textures.hover
							  : ((textures.pressed.is_valid() && is_pressed()) ? textures.pressed
																			   : textures.normal);
				break;
			case DRAW_DISABLED:
				texdraw = textures.disabled.is_valid() ? textures.disabled : textures.normal;
				break;
			}

			Point2 ofs;
			Size2 size;
			bool draw_focus = (has_focus(true) && textures.focused.is_valid());
			bool draw_focus_only = draw_focus && texdraw.is_null();
			if (draw_focus_only) {
				texdraw = textures.focused;
			}

			if (texdraw.is_valid() || textures.click_mask.is_valid()) {
				const Size2 texdraw_size = texdraw.is_valid()
											   ? texdraw->get_size()
											   : Size2(textures.click_mask->get_size());
				size = texdraw_size;
				textures.texture_region = Rect2(Point2(), texdraw_size);
				textures.tile = false;

				switch (textures.stretch_mode) {
				case STRETCH_KEEP:
					break;
				case STRETCH_SCALE:
					size = get_size();
					break;
				case STRETCH_TILE:
					size = get_size();
					textures.tile = true;
					break;
				case STRETCH_KEEP_CENTERED:
					ofs = (get_size() - texdraw_size) / 2;
					break;
				case STRETCH_KEEP_ASPECT_CENTERED:
				case STRETCH_KEEP_ASPECT: {
					Size2 _size = get_size();
					float tex_width = texdraw_size.width * _size.height / texdraw_size.height;
					float tex_height = _size.height;
					if (tex_width > _size.width) {
						tex_width = _size.width;
						tex_height = texdraw_size.height * tex_width / texdraw_size.width;
					}
					if (textures.stretch_mode == STRETCH_KEEP_ASPECT_CENTERED) {
						ofs.x = (_size.width - tex_width) / 2;
						ofs.y = (_size.height - tex_height) / 2;
					}
					size.width = tex_width;
					size.height = tex_height;
				} break;
				case STRETCH_KEEP_ASPECT_COVERED: {
					size = get_size();
					Size2 scale_size = size / texdraw_size;
					float scale = MAX(scale_size.width, scale_size.height);
					Size2 scaled_tex_size = texdraw_size * scale;
					Point2 ofs2 = ((scaled_tex_size - size) / scale).abs() / 2.0f;
					textures.texture_region = Rect2(ofs2, size / scale);
				} break;
				}

				textures.position_rect = Rect2(ofs, size);
				size.width *= textures.hflip ? -1.0f : 1.0f;
				size.height *= textures.vflip ? -1.0f : 1.0f;

				if (!draw_focus_only && texdraw.is_valid()) {
					if (textures.tile) {
						draw_texture_rect(texdraw.ptr(), Rect2(ofs, size), true);
					}
					else {
						draw_texture_rect_region(
							texdraw.ptr(), Rect2(ofs, size), textures.texture_region);
					}
				}
			}
			else {
				textures.position_rect = Rect2();
			}

			if (draw_focus) {
				draw_texture_rect(textures.focused.ptr(), Rect2(ofs, size), false);
			}
		}
		else if (style == STYLE_COLOR_SWATCH) {
			draw_rect(Rect2(Point2(), get_size()), swatch_color);
		}
	} break;
	}
}

void Button::_set_internal_margin(Side p_side, float p_value)
{
	_internal_margin[p_side] = p_value;
}

String Button::_get_translated_text(const String& p_text) const { return String(); }

void Button::_queue_update_size_cache() {}

void Button::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.max_style_size = Vector2();
	theme_cache.style_margin_left = 0;
	theme_cache.style_margin_right = 0;
	theme_cache.style_margin_top = 0;
	theme_cache.style_margin_bottom = 0;

	const bool rtl = is_layout_rtl();
	if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
		_update_style_margins(theme_cache.normal_mirrored);
	}
	else if (theme_cache.normal.is_valid()) {
		_update_style_margins(theme_cache.normal);
	}
	if (has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			_update_style_margins(theme_cache.hover_pressed_mirrored);
		}
		else if (theme_cache.hover_pressed.is_valid()) {
			_update_style_margins(theme_cache.hover_pressed);
		}
	}
	if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
		_update_style_margins(theme_cache.pressed_mirrored);
	}
	else if (theme_cache.pressed.is_valid()) {
		_update_style_margins(theme_cache.pressed);
	}
	if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
		_update_style_margins(theme_cache.hover_mirrored);
	}
	else if (theme_cache.hover.is_valid()) {
		_update_style_margins(theme_cache.hover);
	}
	if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
		_update_style_margins(theme_cache.disabled_mirrored);
	}
	else if (theme_cache.disabled.is_valid()) {
		_update_style_margins(theme_cache.disabled);
	}
	theme_cache.max_style_size = theme_cache.max_style_size.max(
		Vector2(theme_cache.style_margin_left + theme_cache.style_margin_right,
			theme_cache.style_margin_top + theme_cache.style_margin_bottom));
}

Size2 Button::_get_largest_stylebox_size() const { return theme_cache.max_style_size; }

Ref<StyleBox> Button::_get_current_stylebox() const
{
	Ref<StyleBox> stylebox = theme_cache.normal;
	const bool rtl = is_layout_rtl();

	switch (get_draw_mode()) {
	case DRAW_NORMAL: {
		stylebox = (rtl && has_theme_stylebox(SNAME("normal_mirrored")))
					   ? theme_cache.normal_mirrored
					   : theme_cache.normal;
	} break;

	case DRAW_HOVER_PRESSED: {
		if (has_theme_stylebox("hover_pressed")) {
			stylebox = (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored")))
						   ? theme_cache.hover_pressed_mirrored
						   : theme_cache.hover_pressed;
			break;
		}
	}
		[[fallthrough]];
	case DRAW_PRESSED: {
		stylebox = (rtl && has_theme_stylebox(SNAME("pressed_mirrored")))
					   ? theme_cache.pressed_mirrored
					   : theme_cache.pressed;
	} break;

	case DRAW_HOVER: {
		stylebox = (rtl && has_theme_stylebox(SNAME("hover_mirrored")))
					   ? theme_cache.hover_mirrored
					   : theme_cache.hover;
	} break;

	case DRAW_DISABLED: {
		stylebox = (rtl && has_theme_stylebox(SNAME("disabled_mirrored")))
					   ? theme_cache.disabled_mirrored
					   : theme_cache.disabled;
	} break;
	}

	return stylebox;
}

String Button::_get_accessibility_name() const
{
	const String& ac_name = Control::_get_accessibility_name();
	if (!xl_text.is_empty() && ac_name.is_empty()) {
		return xl_text;
	}
	else if (!xl_text.is_empty() && !ac_name.is_empty() && ac_name != xl_text) {
		return ac_name + ": " + xl_text;
	}
	else if (xl_text.is_empty() && ac_name.is_empty() && !get_tooltip_text().is_empty()) {
		return get_tooltip_text();
	}
	else {
		return ac_name;
	}
}

Size2 Button::_fit_icon_size(const Size2& p_size) const
{
	int max_width = theme_cache.icon_max_width;
	Size2 icon_size = p_size;

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

Size2 Button::get_minimum_size_for_text_and_icon(const String& p_text, Ref<Texture2D> p_icon) const
{
	Ref<TextParagraph> paragraph;
	if (p_text.is_empty()) {
		paragraph = text_buf;
	}
	else {
		paragraph.instantiate();
		_shape(paragraph, p_text);
	}

	Size2 minsize = paragraph.is_valid() ? paragraph->get_size() : Size2();
	if (clip_text || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING ||
		autowrap_mode != TextServer::AUTOWRAP_OFF) {
		minsize.width = 0;
	}

	if (!expand_icon && p_icon.is_valid()) {
		Size2 icon_size = _fit_icon_size(p_icon->get_size());
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(minsize.height, icon_size.height);
		}
		else {
			minsize.height += icon_size.height;
		}

		if (horizontal_icon_alignment != HORIZONTAL_ALIGNMENT_CENTER) {
			minsize.width += icon_size.width;
			if (!xl_text.is_empty() || !p_text.is_empty()) {
				minsize.width += MAX(0, theme_cache.h_separation);
			}
		}
		else {
			minsize.width = MAX(minsize.width, icon_size.width);
		}
	}

	if ((!xl_text.is_empty() || !p_text.is_empty()) && theme_cache.font.is_valid()) {
		float font_height = theme_cache.font->get_height(theme_cache.font_size);
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(font_height, minsize.height);
		}
		else {
			minsize.height += font_height;
		}
	}

	Ref<StyleBox> sb = _get_current_stylebox();
	Size2 sb_size = theme_cache.align_to_largest_stylebox
						? _get_largest_stylebox_size()
						: (sb.is_valid() ? sb->get_minimum_size() : Size2());
	return sb_size + minsize;
}

TextServer::OverrunBehavior Button::get_text_overrun_behavior() const { return overrun_behavior; }

String Button::get_text() const { return text; }

void Button::set_text(const String& p_text)
{
	if (text == p_text) {
		return;
	}
	text = p_text;
	xl_text = p_text;
	update_minimum_size();
	queue_redraw();
}

TextServer::AutowrapMode Button::get_autowrap_mode() const { return autowrap_mode; }
void Button::set_autowrap_mode(TextServer::AutowrapMode p_mode) { autowrap_mode = p_mode; }
void Button::set_autowrap_trim_flags(uint32_t p_flags) { autowrap_flags_trim = p_flags; }
uint32_t Button::get_autowrap_trim_flags() const { return autowrap_flags_trim; }
void Button::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	overrun_behavior = p_behavior;
}
void Button::set_text_direction(TextDirection p_text_direction)
{
	text_direction = p_text_direction;
}
Control::TextDirection Button::get_text_direction() const { return text_direction; }
void Button::set_language(const String& p_language) { language = p_language; }
String Button::get_language() const { return language; }

void Button::_update_style_margins(const Ref<StyleBox>& p_stylebox)
{
	if (p_stylebox.is_null()) {
		return;
	}
	theme_cache.max_style_size = theme_cache.max_style_size.max(p_stylebox->get_minimum_size());
	theme_cache.style_margin_left =
		MAX(theme_cache.style_margin_left, p_stylebox->get_margin(SIDE_LEFT));
	theme_cache.style_margin_right =
		MAX(theme_cache.style_margin_right, p_stylebox->get_margin(SIDE_RIGHT));
	theme_cache.style_margin_top =
		MAX(theme_cache.style_margin_top, p_stylebox->get_margin(SIDE_TOP));
	theme_cache.style_margin_bottom =
		MAX(theme_cache.style_margin_bottom, p_stylebox->get_margin(SIDE_BOTTOM));
}

Ref<Texture2D> Button::get_button_icon() const { return icon; }
void Button::set_expand_icon(bool p_enabled) { expand_icon = p_enabled; }
bool Button::is_expand_icon() const { return expand_icon; }
void Button::set_flat(bool p_enabled) { flat = p_enabled; }
bool Button::is_flat() const { return flat; }
void Button::set_clip_text(bool p_enabled) { clip_text = p_enabled; }
bool Button::get_clip_text() const { return clip_text; }
void Button::set_text_alignment(HorizontalAlignment p_alignment) { alignment = p_alignment; }
HorizontalAlignment Button::get_text_alignment() const { return alignment; }
void Button::set_icon_alignment(HorizontalAlignment p_alignment)
{
	horizontal_icon_alignment = p_alignment;
}
void Button::set_vertical_icon_alignment(VerticalAlignment p_alignment)
{
	vertical_icon_alignment = p_alignment;
}
HorizontalAlignment Button::get_icon_alignment() const { return horizontal_icon_alignment; }
VerticalAlignment Button::get_vertical_icon_alignment() const { return vertical_icon_alignment; }

void Button::_shape(Ref<TextParagraph>, String) const {}

Button::Button(const String& p_text, Style p_style)
{
	set_focus_mode(FOCUS_ALL);
	text_buf.instantiate();
	text_buf->set_break_flags(TextServer::BREAK_MANDATORY | autowrap_flags_trim);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_style(p_style);
	set_text(p_text);
}

Button::~Button()
{
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.erase(this);
	}
}

void ButtonGroup::get_buttons(List<Button*>* r_buttons)
{
	for (Button* E : buttons) {
		r_buttons->push_back(E);
	}
}

Button* ButtonGroup::get_pressed_button()
{
	for (Button* E : buttons) {
		if (E->is_pressed()) {
			return E;
		}
	}
	return nullptr;
}

ButtonGroup::ButtonGroup() { set_local_to_scene(true); }

// --- menu_button.cpp ---

void MenuButton::pressed()
{
	if (popup && popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

PopupMenu* MenuButton::get_popup() const { return popup; }

void MenuButton::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuButton::is_switch_on_hover() { return switch_on_hover; }

int MenuButton::get_item_count() const { return popup ? popup->get_item_count() : 0; }

void MenuButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray MenuButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (popup) {
		warnings.append_array(popup->get_configuration_warnings());
	}
	return warnings;
}
#endif

MenuButton::MenuButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_flat(true);
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);

	popup = memnew(PopupMenu);
	popup->hide();
	add_child(popup, false, INTERNAL_MODE_FRONT);
}

MenuButton::~MenuButton() {}

void MenuButton::show_popup() {}

// --- option_button.cpp ---

static const int NONE_SELECTED = -1;

Size2 OptionButton::get_minimum_size() const
{
	Size2 minsize = fit_to_longest_item
						? _cached_size
						: (button ? button->get_minimum_size() : Size2());

	if (has_theme_icon(SNAME("arrow")) && theme_cache.arrow_icon.is_valid()) {
		const Size2 padding = theme_cache.normal.is_valid()
								  ? theme_cache.normal->get_minimum_size()
								  : Size2();
		const Size2 arrow_size = theme_cache.arrow_icon->get_size();

		Size2 content_size = minsize - padding;
		content_size.width += arrow_size.width + MAX(0, theme_cache.h_separation);
		content_size.height = MAX(content_size.height, arrow_size.height);

		minsize = content_size + padding;
	}

	if (fit_to_longest_item && popup) {
		minsize.width = MAX(minsize.width, popup->get_contents_minimum_size().width);
	}

	return minsize;
}

void OptionButton::_selected(int p_which) { _select(p_which, true); }

void OptionButton::pressed()
{
	if (popup && popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

void OptionButton::add_icon_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::add_item(const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_text(int p_idx, const String& p_text)
{
	if (current == p_idx) {
		set_text(p_text);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_id(int p_idx, int p_id) {}

void OptionButton::set_item_tooltip(int p_idx, const String& p_tooltip) {}

void OptionButton::set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode)
{
	if (!popup) {
		return;
	}
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	if (popup->get_item_auto_translate_mode(p_idx) == p_mode) {
		return;
	}
	if (current == p_idx) {
		set_text(popup->get_item_text(p_idx));
	}
	_queue_update_size_cache();
}

String OptionButton::get_item_text(int p_idx) const
{
	return popup ? popup->get_item_text(p_idx) : String();
}

Ref<Texture2D> OptionButton::get_item_icon(int p_idx) const
{
	return popup ? popup->get_item_icon(p_idx) : Ref<Texture2D>();
}

int OptionButton::get_item_id(int p_idx) const
{
	if (p_idx == NONE_SELECTED || !popup) {
		return NONE_SELECTED;
	}

	return popup->get_item_id(p_idx);
}

int OptionButton::get_item_index(int p_id) const
{
	return popup ? popup->get_item_index(p_id) : -1;
}

String OptionButton::get_item_tooltip(int p_idx) const
{
	return popup ? popup->get_item_tooltip(p_idx) : String();
}

Node::AutoTranslateMode OptionButton::get_item_auto_translate_mode(int p_idx) const
{
	return popup ? popup->get_item_auto_translate_mode(p_idx) : AUTO_TRANSLATE_MODE_INHERIT;
}

bool OptionButton::is_item_disabled(int p_idx) const
{
	return popup ? popup->is_item_disabled(p_idx) : false;
}

bool OptionButton::is_item_separator(int p_idx) const
{
	return popup ? popup->is_item_separator(p_idx) : false;
}

bool OptionButton::has_selectable_items() const
{
	for (int i = 0; i < get_item_count(); i++) {
		if (!is_item_disabled(i) && !is_item_separator(i)) {
			return true;
		}
	}
	return false;
}

int OptionButton::get_selectable_item(bool p_from_last) const
{
	if (!p_from_last) {
		for (int i = 0; i < get_item_count(); i++) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	else {
		for (int i = get_item_count() - 1; i >= 0; i--) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	return -1;
}

int OptionButton::get_item_count() const { return popup ? popup->get_item_count() : 0; }

void OptionButton::set_fit_to_longest_item(bool p_fit)
{
	if (p_fit == fit_to_longest_item) {
		return;
	}
	fit_to_longest_item = p_fit;

	_refresh_size_cache();
}

bool OptionButton::is_fit_to_longest_item() const { return fit_to_longest_item; }

void OptionButton::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool OptionButton::get_allow_reselect() const { return allow_reselect; }

bool OptionButton::is_search_bar_enabled() const
{
	return popup ? popup->is_search_bar_enabled() : false;
}

int OptionButton::get_search_bar_min_item_count() const
{
	return popup ? popup->get_search_bar_min_item_count() : 0;
}

void OptionButton::set_search_bar_fuzzy_search_enabled(bool p_enabled)
{
	if (popup) {
		popup->set_search_bar_fuzzy_search_enabled(p_enabled);
	}
}

bool OptionButton::is_search_bar_fuzzy_search_enabled() const
{
	return popup ? popup->is_search_bar_fuzzy_search_enabled() : false;
}

void OptionButton::set_search_bar_fuzzy_search_max_misses(int p_max_misses)
{
	if (popup) {
		popup->set_search_bar_fuzzy_search_max_misses(p_max_misses);
	}
}

int OptionButton::get_search_bar_fuzzy_search_max_misses() const
{
	return popup ? popup->get_search_bar_fuzzy_search_max_misses() : 0;
}

void OptionButton::add_separator(const String& p_text) {}

void OptionButton::_select_int(int p_which)
{
	if (p_which < NONE_SELECTED || !popup) {
		return;
	}
	if (p_which >= popup->get_item_count()) {
		if (!initialized) {
			queued_current = p_which;
		}
		return;
	}
	_select(p_which, false);
}

void OptionButton::select(int p_idx) { _select(p_idx, false); }

int OptionButton::get_selected() const { return current; }

int OptionButton::get_selected_id() const { return get_item_id(current); }

void OptionButton::remove_item(int p_idx)
{
	if (current == p_idx) {
		_select(NONE_SELECTED);
	}
	_queue_update_size_cache();
}

PopupMenu* OptionButton::get_popup() const { return popup; }

void OptionButton::show_popup()
{
	if (!get_viewport() || !popup) {
		return;
	}

	if (current != NONE_SELECTED && !popup->is_item_disabled(current)) {
		popup->scroll_to_item(current);
	}
	else {
		for (int i = 0; i < popup->get_item_count(); i++) {
			if (!popup->is_item_disabled(i)) {
				popup->scroll_to_item(i);
				break;
			}
		}
	}

	Rect2 rect = get_screen_rect();
	rect.position.y += rect.size.height;
	if (get_viewport()->is_embedding_subwindows() && popup->get_force_native()) {
		Transform2D xform = get_viewport()->get_popup_base_transform_native();
		rect = xform.xform(rect);
	}
	rect.size.height = 0;
	popup->set_min_size(Size2(0, 0));
	popup->popup(rect);
}

void OptionButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray OptionButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (popup) {
		warnings.append_array(popup->get_configuration_warnings());
	}
	return warnings;
}
#endif

OptionButton::OptionButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);

	popup = memnew(PopupMenu);
	popup->hide();
	add_child(popup, false, INTERNAL_MODE_FRONT);
}

OptionButton::~OptionButton() {}

void OptionButton::_select(int p_which, bool p_emit) {}

void OptionButton::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon) {}

void OptionButton::clear() {}

void OptionButton::_refresh_size_cache() {}

void OptionButton::_queue_update_size_cache() {}

String OptionButton::_get_translated_text(const String& p_text) const { return p_text; }

// --- menu_bar.cpp ---

bool MenuBar::is_native_menu() const
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return false;
	}
#endif

	return (
		NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU) && prefer_native);
}

void MenuBar::unbind_global_menu()
{
	if (global_menu_tag.is_empty()) {
		return;
	}

	RID main_menu = NativeMenu::get_system_menu(NativeMenu::MAIN_MENU_ID);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = menu_cache.size() - 1; i >= 0; i--) {
		if (!popups[i]->is_system_menu()) {
			if (menu_cache[i].submenu_rid.is_valid()) {
				int item_idx =
					NativeMenu::find_item_index_with_submenu(main_menu, menu_cache[i].submenu_rid);
				if (item_idx >= 0) {
					NativeMenu::remove_item(main_menu, item_idx);
				}
			}
			popups[i]->unbind_global_menu();
			menu_cache.write[i].submenu_rid = RID();
		}
		menu_cache.write[i].sysmenu_id = NativeMenu::INVALID_MENU_ID;
	}

	global_menu_tag = String();
}

int MenuBar::_get_index_at_point(const Point2& p_point) const
{
	Ref<StyleBox> style = theme_cache.normal;
	int offset = 0;
	Point2 point = p_point;
	if (is_layout_rtl()) {
		point.x = get_size().x - point.x;
	}

	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		if (point.x > offset && point.x < offset + size.x) {
			if (point.y > 0 && point.y < size.y) {
				return i;
			}
		}
		offset += size.x + theme_cache.h_separation;
	}
	return -1;
}

Rect2 MenuBar::_get_menu_item_rect(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, menu_cache.size(), Rect2());

	Ref<StyleBox> style = theme_cache.normal;

	int offset = 0;
	for (int i = 0; i < p_index; i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		offset += size.x + theme_cache.h_separation;
	}

	Size2 size = menu_cache[p_index].text_buf->get_size() + style->get_minimum_size();
	if (is_layout_rtl()) {
		return Rect2(Point2(get_size().x - offset - size.x, 0), size);
	}
	else {
		return Rect2(Point2(offset, 0), size);
	}
}

void MenuBar::_draw_menu_item(int p_index)
{
	ERR_FAIL_INDEX(p_index, menu_cache.size());

	RID ci = get_canvas_item();
	bool hovered = (focused_menu == p_index);
	bool pressed = (active_menu == p_index);
	bool rtl = is_layout_rtl();

	if (menu_cache[p_index].hidden) {
		return;
	}

	Color color;
	Ref<StyleBox> style;
	Rect2 item_rect = _get_menu_item_rect(p_index);

	if (menu_cache[p_index].disabled) {
		if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
			style = theme_cache.disabled_mirrored;
		}
		else {
			style = theme_cache.disabled;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_disabled_color;
	}
	else if (hovered && pressed && has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			style = theme_cache.hover_pressed_mirrored;
		}
		else {
			style = theme_cache.hover_pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_hover_pressed_color"))) {
			color = theme_cache.font_hover_pressed_color;
		}
	}
	else if (pressed) {
		if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
			style = theme_cache.pressed_mirrored;
		}
		else {
			style = theme_cache.pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_pressed_color"))) {
			color = theme_cache.font_pressed_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}
	else if (hovered) {
		if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
			style = theme_cache.hover_mirrored;
		}
		else {
			style = theme_cache.hover;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_hover_color;
	}
	else {
		if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
			style = theme_cache.normal_mirrored;
		}
		else {
			style = theme_cache.normal;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		// Focus colors only take precedence over normal state.
		if (has_focus(true)) {
			color = theme_cache.font_focus_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}

	Point2 text_ofs =
		item_rect.position + Point2(style->get_margin(SIDE_LEFT), style->get_margin(SIDE_TOP));

	Color font_outline_color = theme_cache.font_outline_color;
	int outline_size = theme_cache.outline_size;
	if (outline_size > 0 && font_outline_color.a > 0) {
		menu_cache[p_index].text_buf->draw_outline(ci, text_ofs, outline_size, font_outline_color);
	}
	menu_cache[p_index].text_buf->draw(ci, text_ofs, color);
}

int MenuBar::get_menu_idx_from_control(PopupMenu* p_child) const
{
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = 0; i < popups.size(); i++) {
		if (popups[i] == p_child) {
			return i;
		}
	}

	return -1;
}

void MenuBar::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuBar::is_switch_on_hover() { return switch_on_hover; }

void MenuBar::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

Control::TextDirection MenuBar::get_text_direction() const { return text_direction; }

String MenuBar::get_language() const { return language; }

bool MenuBar::is_flat() const { return flat; }

void MenuBar::set_start_index(int p_index)
{
	if (start_index != p_index) {
		start_index = p_index;
		if (!global_menu_tag.is_empty()) {
			unbind_global_menu();
			bind_global_menu();
		}
	}
}

int MenuBar::get_start_index() const { return start_index; }

void MenuBar::set_prefer_global_menu(bool p_enabled)
{
	if (prefer_native != p_enabled) {
		prefer_native = p_enabled;
		if (prefer_native) {
			bind_global_menu
();
		}
		else {
			unbind_global_menu();
		}
	}
}

bool MenuBar::is_prefer_global_menu() const { return prefer_native; }

Size2 MenuBar::get_minimum_size() const
{
	if (is_native_menu()) {
		return Size2();
	}

	Ref<StyleBox> style = theme_cache.normal;

	Vector2 size;
	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 sz = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		size.y = MAX(size.y, sz.y);
		size.x += sz.x;
	}
	if (menu_cache.size() > 1) {
		size.x += theme_cache.h_separation * (menu_cache.size() - 1);
	}
	return size;
}

int MenuBar::get_menu_count() const { return menu_cache.size(); }

String MenuBar::get_menu_title(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].name;
}

String MenuBar::get_menu_tooltip(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].tooltip;
}

void MenuBar::set_menu_disabled(int p_menu, bool p_disabled)
{
	ERR_FAIL_INDEX(p_menu, menu_cache.size());
	menu_cache.write[p_menu].disabled = p_disabled;
	if (!global_menu_tag.is_empty() && menu_cache[p_menu].submenu_rid.is_valid()) {
		RID main_menu = NativeMenu::get_system_menu(NativeMenu::MAIN_MENU_ID);
		int item_idx =
			NativeMenu::find_item_index_with_submenu(main_menu, menu_cache[p_menu].submenu_rid);
		if (item_idx >= 0) {
			NativeMenu::set_item_disabled(main_menu, item_idx, p_disabled);
		}
	}
}

bool MenuBar::is_menu_disabled(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].disabled;
}

bool MenuBar::is_menu_hidden(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].hidden;
}

PopupMenu* MenuBar::get_menu_popup(int p_idx) const
{
	Vector<PopupMenu*> controls = _get_popups();
	if (p_idx >= 0 && p_idx < controls.size()) {
		return controls[p_idx];
	}
	else {
		return nullptr;
	}
}

String MenuBar::get_tooltip(const Point2& p_pos) const
{
	int index = _get_index_at_point(p_pos);
	if (index >= 0 && index < menu_cache.size()) {
		return menu_cache[index].tooltip;
	}
	else {
		return String();
	}
}

MenuBar::MenuBar()
{
	set_focus_mode(FOCUS_ACCESSIBILITY);
	set_process_shortcut_input(true);
}

Vector<PopupMenu*> MenuBar::_get_popups() const {}

void MenuBar::bind_global_menu() {}

void MenuBar::add_child_notify(Node* p_child) {}

void MenuBar::remove_child_notify(Node* p_child) {}

void MenuBar::move_child_notify(Node* p_child) {}
